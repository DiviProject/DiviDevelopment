FROM mongo:5.0.6

SHELL ["/bin/bash", "-c"]
RUN apt-get update
RUN apt-get upgrade -y
RUN apt-get install -y coreutils
RUN apt-get install apt-utils -y
RUN apt-get install bsdmainutils -y
RUN apt-get install software-properties-common -y
RUN apt-get install git -y
RUN apt-get install pkg-config -y
RUN apt-get install libtool -y
RUN apt-get install wget -y
RUN apt-get update
RUN apt-get upgrade -y
RUN apt-get install -y curl
RUN echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections
ENV NVM_DIR /root/.nvm
ENV NODE_VERSION 16.14.2
RUN git clone https://github.com/iquidus/explorer /root/explorer
RUN curl https://raw.githubusercontent.com/creationix/nvm/master/install.sh | bash \
	&& source $NVM_DIR/nvm.sh \
	&& nvm install $NODE_VERSION \
    && nvm alias default $NODE_VERSION \
    && nvm use default \
	&& npm install forever -g \
	&& pushd /root/explorer \
	&& npm install --production \
	&& popd
ENV NODE_PATH $NVM_DIR/v$NODE_VERSION/lib/node_modules
ENV PATH      $NVM_DIR/v$NODE_VERSION/bin:$PATH

RUN touch mongo_init.js
RUN echo 'use explorerdb' >> mongo_init.js
RUN echo 'db.createUser( { user: "iquidus", pwd: "explorer", roles: [ "readWrite" ] } )' >> mongo_init.js
RUN curl -L https://github.com/DiviProject/Divi/releases/download/v2.5.1/divi-2.5.1-x86_64-linux.tar.gz > divi-2.5.1-x86_64-linux.tar.gz
RUN tar xf divi-2.5.1-x86_64-linux.tar.gz
RUN mkdir /root/.divi && touch /root/.divi/divi.conf
RUN echo "rpcport=51475" >> /root/.divi/divi.conf
RUN echo "rpcuser=TempName" >> /root/.divi/divi.conf
RUN echo "rpcpassword=TempSuperPass" >> /root/.divi/divi.conf
RUN echo "daemon=1" >> /root/.divi/divi.conf
RUN echo "staking=0" >> /root/.divi/divi.conf
RUN echo 'cp /root/local_dependencies/settings.json /root/explorer/settings.json && mongo "mongodb://database:27017" < mongo_init.js && pushd /root/explorer && /divi-2.5.1/bin/divid && sleep 10s && npm start' >> ~/.bashrc
ENTRYPOINT ["bash"]