#!/usr/bin/python

import os
import smtplib
import glob
from email.mime.image import MIMEImage
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText

SMTP_SERVER = 'smtp.gmail.com'
SMTP_PORT = 587
sender = os.getenv("SMTP_USER", "sender@email.com")
password = os.getenv("SMTP_PASSWD", "mypassword")
recipient = 'receiver@email.com'
subject = 'Performance results from regression test'
message = 'PFA'

def main():
    msg = MIMEMultipart()
    msg['Subject'] = 'Performance Results'
    msg['To'] = recipient
    msg['From'] = sender

    for filename in glob.glob("*.csv"):
        path = os.path.join(directory, filename)
        if not os.path.isfile(path):
            continue
        text = MIMEImage(open(path, 'rb').read(), _subtype="csv")
        text.add_header('Content-Disposition', 'attachment', filename=filename)
        msg.attach(text)

    part = MIMEText('text', "plain")
    part.set_payload(message)
    msg.attach(part)
    session = smtplib.SMTP(SMTP_SERVER, SMTP_PORT)
    try:
        session.ehlo()
        session.starttls()
        session.ehlo
        session.login(sender, password)
        try:
            session.sendmail(sender, recipient, msg.as_string())
            print 'your message has been sent'
        except Exception, e:
            print 'Unable to send email', e
    finally:
        session.quit()

if __name__ == '__main__':
    main()
