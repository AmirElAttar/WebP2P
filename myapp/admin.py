from django.contrib import admin
from .models import Peer, File, FileAvailability

@admin.register(Peer)
class PeerAdmin(admin.ModelAdmin):
    list_display = ('url', 'last_active')
    search_fields = ('url',)

@admin.register(File)
class FileAdmin(admin.ModelAdmin):
    list_display = ('hash', 'size', 'created_at')
    search_fields = ('filename', 'hash')
    list_filter = ('created_at',)

@admin.register(FileAvailability)
class FileAvailabilityAdmin(admin.ModelAdmin):
    list_display = ('file', 'peer')
    list_filter = ('peer',)
    search_fields = ('file__filename', 'peer__url')