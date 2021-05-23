from flask import Flask, render_template, request, redirect, session, flash, url_for
from .sidebar import sidebar
#render template: passando o nome do modelo e a variáveis ele vai renderizar o template
#request: faz as requisições da nossa aplicação
#redirect: redireciona pra outras páginas
#session: armazena informações do usuário
#flash:mensagem de alerta exibida na tela
#url_for: vai para aonde o redirect indica




def midiPage(name, filename):
    print("the name ")
    sb = sidebar()
    
    
    context = sb.getFile("Arduino_Micro.json")
    print(context)
    return render_template('midi.html',**context)

def keyboardPage(name, filename):
    sb = sidebar()
    context = sb.get()
    return render_template('keyboard.html',**context)


def joystickPage(name, filename):
    sb = sidebar()
    context = sb.get()
    return render_template('joystick.html',**context)


def mousePage(name, filename):
    sb = sidebar()
    context = sb.get()
    return render_template('mouse.html',**context)


def dmxPage(name, filename):
    sb = sidebar()
    context = sb.get()
    return render_template('dmx.html',**context)


def nonamePage(filename):
    context = {
        
    }    
    return render_template('noname.html',**context)

def landPage():
    #read all the files from /conboard/boards
    #parse the xml into variablest to be parsed into the template
    sb = sidebar()
    context = sb.get()
    return render_template('landpage.html',**context)
    #renderizando o template lista e as variáveis desejadas. 


