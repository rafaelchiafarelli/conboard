from flask import Flask, render_template, request, redirect, session, flash, url_for

#render template: passando o nome do modelo e a variáveis ele vai renderizar o template
#request: faz as requisições da nossa aplicação
#redirect: redireciona pra outras páginas
#session: armazena informações do usuário
#flash:mensagem de alerta exibida na tela
#url_for: vai para aonde o redirect indica



def hello():
    return render_template('index.html')
    #renderizando o template lista e as variáveis desejadas. 
