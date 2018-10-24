Author: Jeremy Chen

Dependencies: flask, requests, python3.5

This package contains a Flask-based Python service that allows apps to access the Insta360 Pro through Google's OSC API.
All files under the "pro_osc" directory should be placed on the camera (/system/python/osc), and requires "osc_service.py"
to be launched to initiate the service.

The service supports most functionality from the OSC API, but most notably does not support the following:
- checkForUpdates
- gps features
- meaningful progress updates on in-progress tasks
