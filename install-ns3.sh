#! /bin/bash

#script to auto-install ns-3.  *NO LONGER* assumes that boost is built by source and installed to the home directory under ~/boost/include and ~/lib

#cd /scratch/kebenson
#wget 'http://www.nsnam.org/releases/latest'
wget 'https://www.nsnam.org/release/ns-allinone-3.22.tar.bz2' --no-check-certificate
#wget 'https://www.nsnam.org/release/ns-allinone-3.16.tar.bz2' #latest doesn't have an actual non-version-specific link name
tar -xvjf ns-allinone-3.*.tar.bz2
rm -rf ns-allinone-3.*.tar.bz2

NS3_ALL=`echo $PWD/ns-allinone-3.*`
echo $NS3_ALL
NS3_DIR=`echo $NS3_ALL/ns-3.*`
echo $NS3_DIR
TEMP_DIR=`echo $NS3_DIR/tempns3`
echo $TEMP_DIR

#need to clone the repo
#but into an empty directory
git clone git://github.com/KyleBenson/ns3.git $TEMP_DIR

#move all the git updated ns3 files to the fresh release ns3 folder
#mv won't let me clobber the dirs for w/e reason
cp -R $TEMP_DIR/* $NS3_DIR/
mv -f $TEMP_DIR/.git* $NS3_DIR/ #above doesn't get the hiddens
rm -rf $TEMP_DIR

#now we start building ns-3
cd $NS3_ALL
./build.py #this will fail, but we need to get the other parts built

#some funny build configurations were required when I built boost from source
cd $NS3_DIR
#LDFLAGS_EXTRA="-L$HOME/lib" CXXFLAGS_EXTRA="-I$HOME/boost/include" python waf configure --enable-examples --enable-tests --boost-includes=/home/kebenson/boost/include/ --boost-libs=/home/kebenson/lib/

#bring the latest git version changes in
git checkout geocron
git reset --hard HEAD

./waf configure --enable-examples --enable-tests
./waf build
./test.py
#ln -s ~/ron_output ron_output

#TODO: this reminder automatically...
echo "Don't forget to use the '--with-brite' option after downloading it and recompile!"
