#!/usr/bin/python3

# Traite les donnees du formulaire envoyees par POST

import cgi, os

# FieldStorage contient les donnees envoyees par le formulaire
form = cgi.FieldStorage()

# On extrait le nom de fichier de file_download.html
fileitem = form['filename']

# Check si le fichier a bien été téléchargé
if fileitem.filename:
   open(os.getcwd() + '/cgi-bin/tmp/' + os.path.basename(fileitem.filename), 'wb').write(fileitem.file.read())
   message = 'The file "' + os.path.basename(fileitem.filename) + '" was uploaded to ' + os.getcwd() + '/cgi-bin/tmp'

else:
   message = 'Uploading Failed'

# reponse requete
print("Content-Type: text/html;charset=utf-8")
print ("Content-type:text/html\r\n")
print("<H1> " + message + " </H1>")