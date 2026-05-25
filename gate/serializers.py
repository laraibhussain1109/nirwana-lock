from rest_framework import serializers
from .models import Command, Device, DeviceEvent


class DeviceSerializer(serializers.ModelSerializer):
    class Meta:
        model = Device
        fields = ['id', 'name', 'device_id', 'gate_status', 'finger_status', 'password_mask', 'last_seen']


class CommandSerializer(serializers.ModelSerializer):
    class Meta:
        model = Command
        fields = ['id', 'device', 'command', 'value', 'created_at', 'executed_at']
        read_only_fields = ['created_at', 'executed_at']


class DeviceEventSerializer(serializers.ModelSerializer):
    class Meta:
        model = DeviceEvent
        fields = ['id', 'device', 'message', 'created_at']
        read_only_fields = ['created_at']
