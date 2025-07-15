from rest_framework import status
from rest_framework.decorators import api_view
from rest_framework.response import Response
from rest_framework.exceptions import APIException
from .models import Peer, File, FileAvailability, FileName
from django.utils import timezone
from datetime import timedelta
from django.db.models import Q


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
        "status": "online",
        "peers": total_peers,
        "live_peers": live_peers,
        "files": total_files
    })

@api_view(['POST'])
def register_file(request):  # Changed to accept request parameter
    try:
        data = request.data
        
        # Validate required fields
        required_fields = ['filename', 'hash', 'size', 'peer_url']
        if not all(field in data for field in required_fields):
            return Response(
                {"error": "Missing required fields: filename, hash, size, peer_url"},
                status=status.HTTP_400_BAD_REQUEST
            )
        
        # File registration logic
        file, created = File.objects.get_or_create(
            hash=data['hash'],
            defaults={'size': data['size']}
        )
        
        # Register the filename
        FileName.objects.get_or_create(
            file=file,
            filename=data['filename']
        )
        
        # Register availability
        peer, _ = Peer.objects.get_or_create(url=data['peer_url'])
        FileAvailability.objects.get_or_create(file=file, peer=peer)
        
        return Response({
            "status": "success",
            "hash": file.hash,
            "filename": data['filename'],
            "peer_url": data['peer_url']
        }, status=status.HTTP_201_CREATED)
        
    except Exception as e:
        return Response(
            {"error": str(e)},
            status=status.HTTP_400_BAD_REQUEST
        )

@api_view(['GET'])
def list_files(request):
    search_query = request.GET.get('name', '').strip()
    
    matching_names = FileName.objects.filter(
        filename__icontains=search_query
    ).select_related('file').prefetch_related('file__fileavailability_set__peer')
    
    results = []
    seen_hashes = set()
    
    for name in matching_names:
        file = name.file
        if file.hash in seen_hashes:
            continue
            
        seen_hashes.add(file.hash)
        peers = [avail.peer.url for avail in file.fileavailability_set.all()]
        
        for file_name in file.names.all():
            results.append({
                "filename": file_name.filename,
                "size": file.size,
                "hash": file.hash,
                "peers": peers
            })
    
    return Response({"files": results})