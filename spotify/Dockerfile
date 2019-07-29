FROM tiangolo/uwsgi-nginx-flask:python3.6

ADD requirements.txt .

RUN pip3 install -r requirements.txt

ADD . /app
