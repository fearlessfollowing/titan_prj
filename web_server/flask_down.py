# from flask import request,jsonify,send_from_directory,abort
# import os
#
# def download(filename):
#     if request.method=="GET":
#         if os.path.isfile(os.path.join('upload', filename)):
#             return send_from_directory('upload',filename,as_attachment=True)
#         abort(404)



import requests
try:
    data ={
    'name': 'a.jpg'
    }
    files = {'file': open("/home/vans/桌面/a.jpg", 'rb')}
    url = 'http://127.0.0.1:20000/api/upload'
    response = requests.post(url, data=data, files=files)
except Exception as e:
    print('post exception')