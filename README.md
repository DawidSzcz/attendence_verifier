# Arduino
Kod wgrywany na mikrokontroller arduino

# Attendence Verifier
Podrepozytorium z kodem aplikacji do zarządzania obecnością studentów na wykładzie w php/yii2

URL: http://ec2-44-233-11-175.us-west-2.compute.amazonaws.com:3000
logowanie: konik2/konik1

# Praca
Tekst pracy magisterskiej w tex/pdf

# Student Base
Podrepozytorium z kodem aplikacji do zarządzania danymi studentó w php/symfony4. Udostępnia restowe api, które można odpytać o szczegółowe dane studentów po card_uid albo numerze albumu.
Dodatkowo można się zalogować do tej aplikacji, dodać nowych studenów (można wrzucić dane zbiorczo w pliku jako json) edytować/usunąć istniejących.

URL: http://ec2-44-234-247-226.us-west-2.compute.amazonaws.com:3000
logowanie: adm@adm.com/123456

Format pliku:

[{
    "album_no": "100000",
    "name": "Dawid",
    "surname": "Szczyrk",
    "start_year": 12,
    "semester": 14,
    "card_uid": "11111110"
},
...]
