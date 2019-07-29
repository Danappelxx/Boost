# docker build -t boost-spotify-build .
# docker run --rm -v "$PWD":/var/task boost-spotify-build

FROM lambci/lambda:build-nodejs8.10

# install pip
RUN curl https://bootstrap.pypa.io/get-pip.py | python3 -

RUN mkdir -p /temp/.pypath
WORKDIR /temp

# commands below are cached, only rerun if requirements.txt changes
ADD requirements.txt .

# install dependencies
RUN pip3 install -r requirements.txt -t .pypath
RUN zip -r pypath.zip .pypath

CMD cp pypath.zip /var/task/pypath.zip && echo "Built dependencies and zipped them into pypath.zip"
