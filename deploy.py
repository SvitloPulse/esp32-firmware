import boto3
import os
import json
from pathlib import Path

BUCKET_NAME = os.environ['BUCKET_NAME']
RELEASE_VERSION = os.environ.get('APPVEYOR_BUILD_VERSION', '0.0.0')

artifacts = [
    '.pio/**/*-merged.bin',
    'manifest.json'
]

releases = []

s3_client = boto3.client('s3')

try:
    # Fetch releases.json from bucket into memory
    response = s3_client.get_object(Bucket=BUCKET_NAME, Key='releases.json')
    releases_json = response['Body'].read().decode('utf-8')
    releases = json.loads(releases_json)
except: #s3_client.exceptions.NoSuchKey:
    pass


releases.append(RELEASE_VERSION)
releases = list(set(releases))

with open('releases.json', 'w') as releases_fd:
    json.dump(releases, releases_fd, indent=2)

with open('releases.json', 'rb') as releases_fd:
    s3_client.upload_fileobj(releases_fd, BUCKET_NAME, 'releases.json', ExtraArgs={'ACL':'public-read'})

path = Path('.')

for artifact_pattern in artifacts:
    for artifact in path.glob(artifact_pattern):
        with open(artifact, 'rb') as artifact_fd:
            s3_client.upload_fileobj(artifact_fd, BUCKET_NAME, '/'.join([RELEASE_VERSION, artifact.name]), ExtraArgs={'ACL':'public-read'})
