
 A 'RE6Commander' is very simple command line interface into the 'RateEngine6' database and 
simple test tool for some RateEngine6 working cases. 
For example generate test RatingAccounts and generate test CDRs.

Install need packets (Fedora|CentOS|RedHat):
#yum install php php-pgsql uuid

There are several tasks can be executed with RE6Commander:

I. BillPlans (with Tariff/Prefix/Rate/CalcFunctions) importing from file.

./RE6Commander -i example_importing_settings.csv

  For testing can be used few plans from directory 'test_bp'.This suite from test bill plans are named 'test model 1'.
You can insert all these plans with one command:

./RE6Commander -cp

 If you want to check your BillPlan,can be used follow command for bill plan dumping:

./RE6Commander -dp MobilePromo1

II. RatingAccounts generator

./RE6Commander -c filename , without filename will print 'RatingAccount.username' in the same console

II. CDR generator

./RE6Commander -g calling_number|account_code|src_context|src_tgroup|dst_context|dst_tgroup number

  For testing can be used test CDR server and sample model for CDR records.

This suite from CDR recods are named 'test model 1'.
You can insert all these plans with one command:

./RE6Commander -g1

******************************************************************************************************************************
Въпроси и проблеми:

1. при цена 0лв,ако е нула безплатните минути,ще работи ли ?

2. как се въвежда round_mode ?

3. cdr_server_id,в "3" режим на импорт файловете...??? да има ли такава стойност ...

4. входящият трафик не може да се раздели по сорс префикс(CallingNumber) ... да кажем 359 и различен за да се разпознае 'national' и 'international'

5. test_1 model,няма SMS-и ...

? python3 , cli/console ... ? dkSBC,test cli ...pgsql db ? direct ?
