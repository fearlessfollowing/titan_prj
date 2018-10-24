# from urllib.request import urlopen
# with urlopen('http://www.baidu.com') as response:
#     for line in response:
#         line = line.decode('utf-8')  # Decoding the binary data to text.
#         if 'EST' in line or 'EDT' in line:  # look for Eastern Time
#             print(line)
#
#
# import smtplib
# server = smtplib.SMTP('localhost')
# server.sendmail('wanshun@arashivision.com', 'wanshun@arashivision.com',
# """To: jcaesar@example.org
# From: soothsayer@example.org
#  Beware the Ides of March.
#  """)
# server.quit()


# from flask import Flask
# import logging
# from logging import Formatter

# ADMINS = ['wanshun@arashivision.com']
# app = Flask(__name__)
#
#
# from logging.handlers import SMTPHandler
#
# mail_handler = SMTPHandler('127.0.0.1',
#                            'wanshun@arashivision.com',
#                            ADMINS, 'YourApplication Failed')
# mail_handler.setLevel(logging.WARNING)
#
# mail_handler.setFormatter(Formatter('''
# Message type:       %(levelname)s
# Location:           %(pathname)s:%(lineno)d
# Module:             %(module)s
# Function:           %(funcName)s
# Time:               %(asctime)s
# Version:            %(version)s
#
# Message:
#
# %(message)s
# '''))
#
# app.logger.addHandler(mail_handler)
#
# from logging import getLogger
# loggers = [app.logger, getLogger('sqlalchemy'),
#            getLogger('otherlibrary')]
# for logger in loggers:
#     logger.addHandler(mail_handler)
    # logger.addHandler(file_handler)

# def main():
#     """main function"""
#     print('test mail')
#     print('appinstance ',app.instance_path)
#     app.run(host='0.0.0.0', port=20000, debug=True, use_reloader=False)

# if __name__ == '__main__':
#     main()


#!/usr/bin/python3

import smtplib
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
from email.header import Header

# sender = 'wanshun@arashivision.com'
# receivers = ['wanshun@arashivision.com']
#
# message = """From: From Person <from@fromdomain.com>
# To: To Person <to@todomain.com>
# Subject: SMTP e-mail test
#
# This is a test e-mail message.
# """

# msg_header = 'From: Sender Name <sender@server>\n' \
#              'To: Receiver Name <receiver@server>\n' \
#              'Cc: Receiver2 Name <receiver2@server>\n' \
#              'MIME-Version: 1.0\n' \
#              'Content-type: text/html\n' \
#              'Subject: Any subject\n'
title = 'My title'
msg_content = '<h2>{title} > <font color="green">OK</font></h2>\n'.format(
    title=title)
# msg_full = (''.join([msg_header, msg_content])).encode()

class QQmail(object):
    def __init__(self, server,port):
        self.server = server
        self.port = port
        # self.message = MIMEMultipart()
        self.sender = ''
        self.recevier = []
        self.subject = ''
        self.session = None
        # session.set_debuglevel(True)
        # session.ehlo()M()
        # session.ehlo

    def login(self,email, password, sub = None, recevier = None):
        if self.session is not None:
            print(' self.session is not none')
            self.session.quit()
            self.session = None

        self.sender = email
        self.recevier = recevier or [email]
        self.subject = sub or ' pro mail '

        session = smtplib.SMTP(self.server, self.port)
        session.login(self.sender, password)
        self.session = session

    def attach_file(self,attach):
        # 构造附件1，传送当前目录下的 test.txt 文件
        att1 = MIMEText(open(attach, 'rb').read(), 'base64', 'utf-8')
        att1["Content-Type"] = 'application/octet-stream'
        # 这里的filename可以任意写，写什么名字，邮件中显示什么名字
        att1["Content-Disposition"] = 'attachment; filename="test.txt"'
        # message.attach(att1)

        return att1

    def send_message(self, subject , body = None , attach = None):
        message = MIMEMultipart()
        message['From'] = Header(self.sender, 'utf-8')
        message['To'] = Header(str(self.recevier), 'utf-8')
        message['Subject'] = Header(subject, 'utf-8')
        message.attach(MIMEText(body or 'test mail', 'plain', 'utf-8'))
        if attach is not None:
            message.attach(self.attach_file(attach))
        self.session.sendmail(
            self.sender,
            self.recevier,
            message.as_string())

        # headers = [
        #     "From: " + self.sender,
        #     "Subject: " + subject,
        #     "To: " + self.sender,
        #     "MIME-Version: 1.0",
        #    "Content-Type: text/html"]
        # headers = "\r\n".join(headers)
        # dest = [self.sender]
        # print('send msg dest ',dest)
        # self.session.sendmail(
        #     self.sender,
        #     self.recevier,
        #     headers + '\r\n' + body)
        print('send mail successfully')


#邮件正文内容


# 构造附件2，传送当前目录下的 runoob.txt 文件
# att2 = MIMEText(open('runoob.txt', 'rb').read(), 'base64', 'utf-8')
# att2["Content-Type"] = 'application/octet-stream'
# att2["Content-Disposition"] = 'attachment; filename="runoob.txt"'
# message.attach(att2)

sm = QQmail('smtp.126.com',25)
sm.login('txuenihao@126.com', 'tx123456')
sm.send_message(title,msg_content,'tmp')