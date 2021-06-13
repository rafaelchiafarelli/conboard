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
    return {"value":"midipage value"}

def keyboardPage(name, filename):
    sb = sidebar()
    context = sb.get()
    return {"value":"kayboardpage value"}


def joystickPage(name, filename):
    sb = sidebar()
    context = sb.get()
    return {"value":"joystickpage value"}


def mousePage(name, filename):
    sb = sidebar()
    context = sb.get()
    return {"value":"mousedpage value"}


def dmxPage(name, filename):
    sb = sidebar()
    context = sb.get()
    return {"value":"dmxpage value"}


def nonamePage(filename):
    context = {
        
    }    
    return {"value":"nonamepage value"}

def landPage():
    #read all the files from /conboard/boards
    #parse the xml into variablest to be parsed into the template
    sb = sidebar()
    context = sb.get()
    return {"value":"landpage value"}
    #renderizando o template lista e as variáveis desejadas. 


