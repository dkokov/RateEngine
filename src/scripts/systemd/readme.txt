https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/7/html/system_administrators_guide/sect-managing_services_with_systemd-unit_files

cp -v RateEngine.service /etc/systemd/system

systemctl enable RateEngine

systemctl start RateEngine
systemctl status RateEngine
systemctl stop RateEngine
