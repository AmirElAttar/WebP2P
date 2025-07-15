from django.core.management.base import BaseCommand
from django.utils.timezone import make_aware
from datetime import datetime
from myapp.models import Peer, File, FileName, FileAvailability

class Command(BaseCommand):
    help = 'Loads sample data into the database'
    
    def handle(self, *args, **options):
        self.stdout.write("Loading sample data...")
        
        # Clear existing data
        FileAvailability.objects.all().delete()
        FileName.objects.all().delete()
        File.objects.all().delete()
        Peer.objects.all().delete()
        
        # Sample Peer data
        peers = [
            {"url": "http://peer1.example.com", "last_active": "2023-01-15T10:00:00"},
            {"url": "http://peer2.example.org", "last_active": "2023-01-16T11:30:00"},
            {"url": "http://peer3.test.net", "last_active": "2023-01-17T09:15:00"},
            {"url": "http://peer4.demo.com", "last_active": "2023-01-18T14:45:00"},
            {"url": "http://peer5.example.io", "last_active": "2023-01-19T16:20:00"},
        ]
        
        # Sample File data - same hash can have multiple filenames
        files = [
            {
                "hash": "a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6q7r8s9t0",
                "size": 1024000,
                "names": ["document.pdf", "report.pdf"]
            },
            {
                "hash": "b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6q7r8s9t0u1",
                "size": 5120000,
                "names": ["presentation.pptx", "slides.pptx"]
            },
            {
                "hash": "c3d4e5f6g7h8i9j0k1l2m3n4o5p6q7r8s9t0u1v2",
                "size": 256000,
                "names": ["image.jpg"]
            },
        ]
        
        # Create Peers
        peer_objects = []
        for peer_data in peers:
            peer = Peer(
                url=peer_data['url'],
                last_active=make_aware(datetime.strptime(peer_data['last_active'], "%Y-%m-%dT%H:%M:%S"))
            )
            peer.save()
            peer_objects.append(peer)
        
        # Create Files and their names
        file_objects = []
        for file_data in files:
            file, created = File.objects.get_or_create(
                hash=file_data['hash'],
                defaults={'size': file_data['size']}
            )
            
            for filename in file_data['names']:
                FileName.objects.get_or_create(
                    file=file,
                    filename=filename
                )
            
            file_objects.append(file)
        
        # Create FileAvailability relationships
        availability_data = [
            {"file": 0, "peer": 0},  # document.pdf/report.pdf on peer1
            {"file": 0, "peer": 1},  # document.pdf/report.pdf on peer2
            {"file": 1, "peer": 0},  # presentation.pptx/slides.pptx on peer1
            {"file": 1, "peer": 2},  # presentation.pptx/slides.pptx on peer3
            {"file": 2, "peer": 1},  # image.jpg on peer2
            {"file": 2, "peer": 3},  # image.jpg on peer4
        ]
        
        for avail_data in availability_data:
            FileAvailability.objects.get_or_create(
                file=file_objects[avail_data['file']],
                peer=peer_objects[avail_data['peer']]
            )
        
        self.stdout.write(self.style.SUCCESS('Successfully loaded sample data'))