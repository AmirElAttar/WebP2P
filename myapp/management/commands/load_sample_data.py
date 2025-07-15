from django.core.management.base import BaseCommand
from django.utils.timezone import make_aware
from datetime import datetime
from myapp.models import Peer, File, FileAvailability

class Command(BaseCommand):
    help = 'Loads sample data into the database'
    
    def handle(self, *args, **options):
        self.stdout.write("Loading sample data...")
        
        # Clear existing data
        FileAvailability.objects.all().delete()
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
        
        # Sample File data
        files = [
            {"filename": "document.pdf", "size": 1024000, "hash": "a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6q7r8s9t0"},
            {"filename": "presentation.pptx", "size": 5120000, "hash": "b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6q7r8s9t0u1"},
            {"filename": "image.jpg", "size": 256000, "hash": "c3d4e5f6g7h8i9j0k1l2m3n4o5p6q7r8s9t0u1v2"},
            {"filename": "video.mp4", "size": 20480000, "hash": "d4e5f6g7h8i9j0k1l2m3n4o5p6q7r8s9t0u1v2w3"},
            {"filename": "archive.zip", "size": 4096000, "hash": "e5f6g7h8i9j0k1l2m3n4o5p6q7r8s9t0u1v2w3x4"},
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
        
        # Create Files
        file_objects = []
        for file_data in files:
            file = File(
                filename=file_data['filename'],
                size=file_data['size'],
                hash=file_data['hash']
            )
            file.save()
            file_objects.append(file)
        
        # Create FileAvailability relationships
        availability_data = [
            {"file": 0, "peer": 0},  # document.pdf on peer1
            {"file": 0, "peer": 1},  # document.pdf on peer2
            {"file": 1, "peer": 0},  # presentation.pptx on peer1
            {"file": 1, "peer": 2},  # presentation.pptx on peer3
            {"file": 2, "peer": 1},  # image.jpg on peer2
            {"file": 2, "peer": 3},  # image.jpg on peer4
            {"file": 3, "peer": 2},  # video.mp4 on peer3
            {"file": 3, "peer": 4},  # video.mp4 on peer5
            {"file": 4, "peer": 0},  # archive.zip on peer1
            {"file": 4, "peer": 4},  # archive.zip on peer5
        ]
        
        for avail_data in availability_data:
            FileAvailability.objects.create(
                file=file_objects[avail_data['file']],
                peer=peer_objects[avail_data['peer']]
            )
        
        self.stdout.write(self.style.SUCCESS('Successfully loaded sample data'))