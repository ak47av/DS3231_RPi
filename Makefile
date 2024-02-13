REMOTE_USER="arun"
REMOTE_HOST="192.168.1.200"
REMOTE_PATH="/home/arun/Documents/assgt1"
directory=/Users/arun/Documents/UNIVERSITY/EE513_Connected_Embedded/assignment_1/DS3231_RPI

upload:
	rsync -avz --progress $(directory) "arun@${REMOTE_HOST}:${REMOTE_PATH}"