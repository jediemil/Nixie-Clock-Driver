DIR=$(dirname $0)
echo Directory is $DIR
cd $DIR

echo "Compiling"
pio run
if [ $? != 0 ]; then
  echo "Compile failed"
  read
  exit 1
fi
echo "Compile complete"
echo "Uploading"
curl -X POST -F 'update=@.pio/build/esp32dev/firmware.bin' http://klocka.local/update
echo "Upload done"
sleep 4