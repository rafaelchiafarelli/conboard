from flask import Flask, render_template, request, redirect, session, flash, url_for
from .sidebar import sidebar


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
    return render_template("index.html", token = "hello something")
    #renderizando o template lista e as variáveis desejadas. 


