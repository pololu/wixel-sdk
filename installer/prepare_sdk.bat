:: prepare_wixel_sdk_for_bundle.bat clones the Wixel SDK,
:: and builds the docs so it can be put in to the Wixel
:: Development Bundle.

cd %~dp0/build
call git clone -o pololu git://github.com/pololu/wixel-sdk.git
cd wixel-sdk
call git config core.autocrlf true
pause
make docs
cd ..