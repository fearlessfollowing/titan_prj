from werkzeug.utils import secure_filename
from flask import Flask,render_template,jsonify,request,send_from_directory
import time
import os
import base64

app = Flask(__name__)
UPLOAD_FOLDER='upload'
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER
basedir = os.path.abspath(os.path.dirname(__file__))
ALLOWED_EXTENSIONS = set(['txt','png','jpg','xls','JPG','PNG','xlsx','gif','GIF'])

def allowed_file(filename):
    return '.' in filename and filename.rsplit('.',1)[1] in ALLOWED_EXTENSIONS

@app.route('/test/upload')
def upload_test():
    return render_template('upload.html')

@app.route('/test/download')
def download_test():
    return render_template('download.html')

@app.route('/api/download/<filename>',methods=['GET'])
def download(filename):
    print('filename ',filename)
    if request.method == "GET":
        file_dir = os.path.join(basedir, app.config['UPLOAD_FOLDER'])
        print('file_dir ',file_dir)
        #print('__file__ ',__file__)
        if os.path.isfile(os.path.join(file_dir, filename)):
            print('download filename ',filename)
            return send_from_directory('upload', filename, as_attachment=True)
        # abort(404)
    return '0'

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=20000, debug=True)