from django.db import migrations, models
import django.db.models.deletion


class Migration(migrations.Migration):
    initial = True

    dependencies = []

    operations = [
        migrations.CreateModel(
            name='Device',
            fields=[
                ('id', models.BigAutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('name', models.CharField(max_length=100)),
                ('device_id', models.CharField(max_length=80, unique=True)),
                ('token', models.CharField(max_length=120)),
                ('gate_status', models.CharField(default='UNKNOWN', max_length=30)),
                ('finger_status', models.CharField(default='READY', max_length=30)),
                ('password_mask', models.CharField(default='******', max_length=20)),
                ('last_seen', models.DateTimeField(blank=True, null=True)),
            ],
        ),
        migrations.CreateModel(
            name='Command',
            fields=[
                ('id', models.BigAutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('command', models.CharField(choices=[('OPEN', 'Open'), ('CLOSE', 'Close'), ('PASSWORD', 'Password'), ('ENROLL', 'Enroll'), ('DELETE', 'Delete')], max_length=20)),
                ('value', models.CharField(blank=True, default='', max_length=60)),
                ('created_at', models.DateTimeField(auto_now_add=True)),
                ('executed_at', models.DateTimeField(blank=True, null=True)),
                ('device', models.ForeignKey(on_delete=django.db.models.deletion.CASCADE, related_name='commands', to='gate.device')),
            ],
            options={'ordering': ['-created_at']},
        ),
        migrations.CreateModel(
            name='DeviceEvent',
            fields=[
                ('id', models.BigAutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('message', models.CharField(max_length=200)),
                ('created_at', models.DateTimeField(auto_now_add=True)),
                ('device', models.ForeignKey(on_delete=django.db.models.deletion.CASCADE, related_name='events', to='gate.device')),
            ],
            options={'ordering': ['-created_at']},
        ),
    ]
