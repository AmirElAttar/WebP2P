from django.core.management.base import BaseCommand
from django.utils import timezone
from datetime import timedelta
from myapp.models import Peer

class Command(BaseCommand):
    help = 'Removes peers inactive for more than 30 minutes'

    def handle(self, *args, **kwargs):
        threshold = timezone.now() - timedelta(minutes=30)
        old_peers = Peer.objects.filter(last_active__lt=threshold)
        count = old_peers.count()
        old_peers.delete()
        self.stdout.write(f"Deleted {count} inactive peers")