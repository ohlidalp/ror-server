cd ../../../

cmake -DCMAKE_INSTALL_PREFIX:STRING=/usr \
-DRORSERVER_NO_STACKLOG:BOOL=ON \
-DRORSERVER_CRASHHANDLER:BOOL=OFF \
-DRORSERVER_GUI:BOOL=OFF \
-DRORSERVER_WITH_ANGELSCRIPT:BOOL=OFF \
-DRORSERVER_WITH_WEBSERVER:BOOL=OFF \
.

make -j2
