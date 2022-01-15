# Explanations
This module has the responsability to connect to the user, 
Websockets are not handled by this module
IO actions are not handled by this module

# User API
* The user API's are responsible to get information regarding the user and it's activities. User actions are handled by another API's.

Get user profile
Set user profile
    name, pic, profile_pic, email, phone, occupation, education, description, share_status, visible_status, etc
Get statistics from the user
    when was the last time user was connected
    how long is this user connected

Get user activities
    what ware the last xx user actions?

# Security API
* Security API are those who enable a localy connected user to access the device. In order to connect to the pages, user must insert a password that is shown in the screen. Every time a login is attempted a new password is shown. In case the user is operating the screen and a login action occurs, the login takes precedence, but, it three failed attempts are performed, login is shutdown and the edge of the screen will turn red and the password get more complicated. Rebooting the system will restart the count, however, after 10 failed attempts. The system will not allow connection unless a password is inserted physicaly in the keyboard. This password is sent to the user connected to the device. 

Login with power password
Send password to screen
Send email notification to the user?

# Shared API
Get shared files 
    share specific per file
    how many times it was shared
Share a file
UnShare a file

# Vault API
Clear Vault
Add file to the vault
Retrieve file from the vault

# 