from flask import Flask, render_template, request, redirect, session, flash, url_for
from xml.dom import minidom
import glob
#render template: passando o nome do modelo e a variáveis ele vai renderizar o template
#request: faz as requisições da nossa aplicação
#redirect: redireciona pra outras páginas
#session: armazena informações do usuário
#flash:mensagem de alerta exibida na tela
#url_for: vai para aonde o redirect indica




def midi():
    context = {

    }
    return render_template('midi.html',**context)


def keyboard():
    context = {
        
    }
    return render_template('keyboard.html',**context)


def joystick():
    context = {
        
    }    
    return render_template('joystick.html',**context)


def mouse():
    context = {
        
    }    
    return render_template('mouse.html',**context)


def dmx():
    context = {
        
    }    
    return render_template('dmx.html',**context)


def noname():
    context = {
        
    }    
    return render_template('noname.html',**context)

def landpage():
    #read all the files from /conboard/boards
    #parse the xml into variablest to be parsed into the template
    xml_files = glob.glob("/conboard/boards/*.xml")
    context = {}
    midis = []
    keyboards = []
    dmxs = []
    mice = []
    joysticks = []
    none_type = []
    for xml in xml_files:
        xmldoc = minidom.parse(xml)
        device = xmldoc.getElementById('DEVICE')
        if device is not None:
            if 'type' in device:
                if device['type'] == 'midi':
                    midis.append(device)
                elif device['type'] == 'keyboard':
                    keyboards.append(device)
                elif device['type'] == 'DMX':
                    dmxs.append(device)
                elif device['type'] == 'mouse':
                    mice.append(device)
                elif device['type'] == 'joystick':
                    joysticks.append(device)
                else:
                    none_type.append(device)
    context = {
        'midis':midis,
        'keyboards':keyboards,
        'dmxs':dmxs,
        'joysticks':joysticks
    }
    return render_template('landpage.html',**context)
    #renderizando o template lista e as variáveis desejadas. 
