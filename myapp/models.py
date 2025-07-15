from django.db import models
from django.utils import timezone

class Peer(models.Model):
    url = models.CharField(max_length=255, unique=True)
    last_active = models.DateTimeField(default=timezone.now)

    def __str__(self):
        return self.url

    class Meta:
        ordering = ['-last_active']
        
class File(models.Model):
    size = models.BigIntegerField()
    hash = models.CharField(max_length=64, unique=True, primary_key=True)  # SHA256
    created_at = models.DateTimeField(default=timezone.now)

    def __str__(self):
        return f"{self.filename} ({self.size} bytes)"

    class Meta:
        ordering = ['-created_at']

class FileName(models.Model):
    file = models.ForeignKey(File, on_delete=models.CASCADE, related_name='names')
    filename = models.CharField(max_length=255)
    uploaded_at = models.DateTimeField(default=timezone.now)

    class Meta:
        unique_together = [('file', 'filename')]  # Each filename can only map to a file once

class FileAvailability(models.Model):
    file = models.ForeignKey(File, on_delete=models.CASCADE)
    peer = models.ForeignKey(Peer, on_delete=models.CASCADE)
    
    def __str__(self):
        return f"{self.file.filename} available on {self.peer.url}"

    class Meta:
        unique_together = [('file', 'peer')]
        verbose_name_plural = "File Availabilities"