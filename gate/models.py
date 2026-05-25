from django.db import models


class Device(models.Model):
    name = models.CharField(max_length=100)
    device_id = models.CharField(max_length=80, unique=True)
    token = models.CharField(max_length=120)
    gate_status = models.CharField(max_length=30, default='UNKNOWN')
    finger_status = models.CharField(max_length=30, default='READY')
    password_mask = models.CharField(max_length=20, default='******')
    last_seen = models.DateTimeField(null=True, blank=True)

    def __str__(self):
        return f"{self.name} ({self.device_id})"


class Command(models.Model):
    COMMAND_CHOICES = [
        ('OPEN', 'Open'),
        ('CLOSE', 'Close'),
        ('PASSWORD', 'Password'),
        ('ENROLL', 'Enroll'),
        ('DELETE', 'Delete'),
    ]

    device = models.ForeignKey(Device, on_delete=models.CASCADE, related_name='commands')
    command = models.CharField(max_length=20, choices=COMMAND_CHOICES)
    value = models.CharField(max_length=60, blank=True, default='')
    created_at = models.DateTimeField(auto_now_add=True)
    executed_at = models.DateTimeField(null=True, blank=True)

    class Meta:
        ordering = ['-created_at']


class DeviceEvent(models.Model):
    device = models.ForeignKey(Device, on_delete=models.CASCADE, related_name='events')
    message = models.CharField(max_length=200)
    created_at = models.DateTimeField(auto_now_add=True)

    class Meta:
        ordering = ['-created_at']
