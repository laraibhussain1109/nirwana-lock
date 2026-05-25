from django.urls import path
from . import views

urlpatterns = [
    path('', views.dashboard, name='dashboard'),
    path('api/devices/', views.devices, name='devices'),
    path('api/devices/<str:device_id>/commands/', views.create_command, name='create_command'),
    path('api/cloud/next-command/', views.next_command, name='next_command'),
    path('api/cloud/heartbeat/', views.heartbeat, name='heartbeat'),
]
