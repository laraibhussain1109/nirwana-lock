from django.shortcuts import get_object_or_404, render
from django.utils import timezone
from django.contrib.auth.decorators import login_required
from rest_framework import status
from rest_framework.decorators import api_view
from rest_framework.decorators import permission_classes
from rest_framework.permissions import IsAuthenticated
from rest_framework.response import Response

from .models import Command, Device, DeviceEvent
from .serializers import CommandSerializer, DeviceSerializer


@login_required(login_url='/admin/login/')
def dashboard(request):
    devices = Device.objects.all()
    return render(request, 'dashboard.html', {'devices': devices})


@api_view(['GET'])
@permission_classes([IsAuthenticated])
def devices(request):
    return Response(DeviceSerializer(Device.objects.all(), many=True).data)


@api_view(['POST'])
@permission_classes([IsAuthenticated])
def create_command(request, device_id):
    device = get_object_or_404(Device, device_id=device_id)
    payload = request.data.copy()
    payload['device'] = device.id
    serializer = CommandSerializer(data=payload)
    serializer.is_valid(raise_exception=True)
    command = serializer.save()
    DeviceEvent.objects.create(device=device, message=f"Queued {command.command} {command.value}".strip())
    return Response(CommandSerializer(command).data, status=status.HTTP_201_CREATED)


@api_view(['GET'])
def next_command(request):
    device_id = request.query_params.get('device_id', '')
    token = request.query_params.get('token', '')
    device = get_object_or_404(Device, device_id=device_id, token=token)
    device.last_seen = timezone.now()
    device.save(update_fields=['last_seen'])

    cmd = device.commands.filter(executed_at__isnull=True).order_by('created_at').first()
    if not cmd:
        return Response({'command': 'NONE', 'value': ''})

    cmd.executed_at = timezone.now()
    cmd.save(update_fields=['executed_at'])
    return Response({'command': cmd.command, 'value': cmd.value})


@api_view(['POST'])
def heartbeat(request):
    device_id = request.data.get('device_id', '')
    token = request.data.get('token', '')
    device = get_object_or_404(Device, device_id=device_id, token=token)

    device.gate_status = request.data.get('gate_status', device.gate_status)
    device.finger_status = request.data.get('finger_status', device.finger_status)
    device.password_mask = request.data.get('password_mask', device.password_mask)
    device.last_seen = timezone.now()
    device.save(update_fields=['gate_status', 'finger_status', 'password_mask', 'last_seen'])

    DeviceEvent.objects.create(device=device, message='Heartbeat')
    return Response({'status': 'ok'})
