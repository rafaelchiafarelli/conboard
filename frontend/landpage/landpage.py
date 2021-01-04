from flask import Flask, render_template, request, redirect, session, flash, url_for
from xml.dom import minidom
import glob
#render template: passando o nome do modelo e a variáveis ele vai renderizar o template
#request: faz as requisições da nossa aplicação
#redirect: redireciona pra outras páginas
#session: armazena informações do usuário
#flash:mensagem de alerta exibida na tela
#url_for: vai para aonde o redirect indica



def hello():
    #read all the files from /conboard/boards
    #parse the xml into variablest to be parsed into the template
    xml_files = glob.glob("/conboard/boards/*.xml")
    context = {}
    midis = []
    keyboards = []
    dmxs = []
    mice = []
    joysticks = []
    for xml in xml_files:
        xmldoc = minidom.parse(xml)
        device = xmldoc.getElementById('DEVICE')
        if device is not None:
            if 'type' in device:
                if device['type'] == 'midi':
                    midis.append(device)
                if device['type'] == 'keyboard':
                    keyboards.append(device)
                    
                if device['type'] == 'DMX':
                    dmxs.append(device)
                if device['type'] == 'mouse':
                    mice.append(device)
                if device['type'] == 'joystick':
                    joysticks.append(device)

    return render_template('index.html')
    #renderizando o template lista e as variáveis desejadas. 
