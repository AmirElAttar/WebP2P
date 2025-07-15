from rest_framework import status
from rest_framework.decorators import api_view
from rest_framework.response import Response
from rest_framework.exceptions import APIException
from .models import Peer, File, FileAvailability
from django.utils import timezone
from datetime import timedelta


class FileAlreadyRegistered(APIException):
    status_code = 409
    default_detail = 'File already registered'

@api_view(['GET'])
def api_status(request):
    total_peers = Peer.objects.count()
    total_files = File.objects.count()

    fifteen_minutes_ago = timezone.now() - timedelta(minutes=15)
    live_peers = Peer.objects.filter(last_active__gte=fifteen_minutes_ago).count()

    return Response({
        "status": "online",  # backend is alive
        "peers": total_peers,
        "live_peers": live_peers,
        "files": total_files
    })

@api_view(['POST'])
def register_file(request):
    required_fields = ['filename', 'size', 'hash', 'peer_url']
    if not all(field in request.data for field in required_fields):
        return Response({"error": "Missing required field"}, status=status.HTTP_400_BAD_REQUEST)
    
    # Validate SHA256 hash length
    if len(request.data['hash']) != 64:
        return Response({"error": "Invalid hash length"}, status=status.HTTP_400_BAD_REQUEST)
    
    # Get or create peer
    peer, _ = Peer.objects.update_or_create(
        url=request.data['peer_url'],
        defaults={'last_active': timezone.now()}
    )
    
    # Get or create file
    file_obj, created = File.objects.get_or_create(
        filename=request.data['filename'],
        size=request.data['size'],
        hash=request.data['hash']
    )
    
    # Create file availability
    FileAvailability.objects.get_or_create(file=file_obj, peer=peer)
    if not created:
        raise FileAlreadyRegistered()
    
    return Response({"success": True, "file_id": file_obj.id})

@api_view(['GET'])
def list_files(request):
    files = []
    for file in File.objects.prefetch_related('fileavailability_set__peer').all():
        peers = [avail.peer.url for avail in file.fileavailability_set.all()]
        files.append({
            "filename": file.filename,
            "size": file.size,
            "hash": file.hash,
            "peers": peers
        })
    return Response({"files": files})