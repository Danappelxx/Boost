FROM tiangolo/uwsgi-nginx-flask:python3.6

ADD spotify/requirements.txt .

RUN pip3 install -r requirements.txt

ADD spotify /app
