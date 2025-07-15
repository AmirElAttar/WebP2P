from rest_framework import serializers
from .models import SharedFile, Peer

class PeerSerializer(serializers.ModelSerializer):
    class Meta:
        model = Peer
        fields = ["ip", "last_heartbeat"]

class FileSerializer(serializers.ModelSerializer):
    peer = PeerSerializer(read_only=True)

    class Meta:
        model = SharedFile
        fields = ["filename", "sha256", "size", "peer", "last_seen"]