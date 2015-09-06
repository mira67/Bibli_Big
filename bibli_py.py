"""
User interface for Bibli Remote Control through Wifi, Text Message to Arduino
and sound player.

Note:
1. Make sure the gifs are in same directory as this code
2. Make sure change the 'sounds_path' to your sounds path
3. Double click item in list will play the audio

Author: Qi LIU, qliu.hit@gmail.com
Date: 05/2015
"""
from Tkinter import *
import pygame as pg
import socket
import os

UDP_IP = "10.0.1.2"
#UDP_IP = "192.168.43.211"
UDP_PORT = 2390

print "UDP target IP:", UDP_IP
print "UDP target port:", UDP_PORT

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP

#sound directory
#sounds_path = "/Users/jalalihartman/Documents/BiBliCode/funny_sounds/"
#image_path = "/Users/jalalihartman/Documents/BiBliCode/images/"
sounds_path = "/Users/mira67/Google Drive/Comic Con Robot/Arduino_iCreate/Bibli_Code_v4.0/funny_sounds/"
image_path = "/Users/mira67/Google Drive/Comic Con Robot/Arduino_iCreate/Bibli_Code_v4.0/images/"

class App:
  def __init__(self, master):
    #control panel
    fctrl = Frame(master,width = 450, height = 100,bd=1)
    fctrl.grid_propagate(False)
    fctrl.grid(row=0,column=0)
    self.buttonf = Button(fctrl, 
                         text="Forward", fg="blue",
                         command=self.rforward).grid(row=0,column=2)
    self.buttonb = Button(fctrl,
                         text="Back",fg="blue",
                         command=self.rback).grid(row=2,column=2)
    self.buttonl = Button(fctrl, 
                         text="Left", fg="blue",
                         command=self.rleft).grid(row=1,column=1)
    self.buttonr = Button(fctrl,
                         text="Right",fg="blue",
                         command=self.rright).grid(row=1,column=3)
    self.buttons = Button(fctrl, 
                         text="Stop", fg="red",
                         command=self.rstop).grid(row=1,column=2)
    print self.buttons

    #text message panel
    fmsg = Frame(master,width = 300, height = 100)
    fmsg.grid(row=0,column=1)
    self.label = Label(fmsg, text="Message:")
    self.label.grid(row=0,column=0)
    self.e1 = Entry(fmsg)
    self.e1.grid(row=0,column=1)
    self.sendmsg = Button(fmsg, text='Send Msg', command=self.send_msg).grid(row=1,column=0)


    #play sounds
    fsnd = Frame(master,width = 450, height = 450,relief=RIDGE)
    fsnd.grid(row=1,column=0)

    self.photo1=PhotoImage(file= image_path + "darth.gif")
    self.s1=Button(fsnd,command = lambda: self.play_sound("darthvader.wav"),justify = LEFT)
    self.s1.config(image=self.photo1,width="150",height="150")
    self.s1.grid(row=0,column=0)

    self.photo2=PhotoImage(file= image_path + "girl_laugh.gif")
    self.s2=Button(fsnd,command = lambda: self.play_sound("laugh.wav"),justify = LEFT)
    self.s2.config(image=self.photo2,width="150",height="150")
    self.s2.grid(row=0,column=1)

    self.photo3=PhotoImage(file= image_path + "cat_meow.gif")
    self.s3=Button(fsnd,command = lambda:self.play_sound("Cat_Meow.wav"),justify = LEFT)
    self.s3.config(image=self.photo3,width="150",height="150")
    self.s3.grid(row=0,column=2)

    self.photo4=PhotoImage(file= image_path + "hold_me.gif")
    self.s4=Button(fsnd,command = lambda: self.play_sound("Hold_Me_Daddy.wav"),justify = LEFT)
    self.s4.config(image=self.photo4,width="150",height="150")
    self.s4.grid(row=1,column=0)
    
    self.photo5=PhotoImage(file= image_path + "iloveyou.gif")
    self.s5=Button(fsnd,command = lambda:self.play_sound("iloveyou.wav"),justify = LEFT)
    self.s5.config(image=self.photo5,width="150",height="150")
    self.s5.grid(row=1,column=1)

    self.photo6=PhotoImage(file= image_path + "r2d2.gif")
    self.s6=Button(fsnd,command = lambda: self.play_sound("r2d2.wav"),justify = LEFT)
    self.s6.config(image=self.photo6,width="150",height="150")
    self.s6.grid(row=1,column=2)
    
    self.photo7=PhotoImage(file= image_path + "punch.gif")
    self.s7=Button(fsnd,command = lambda:self.play_sound("punch.wav"),justify = LEFT)
    self.s7.config(image=self.photo7,width="150",height="150")
    self.s7.grid(row=2,column=0)

    self.photo8=PhotoImage(file= image_path + "laser.gif")
    self.s8=Button(fsnd,command = lambda: self.play_sound("laser.wav"),justify = LEFT)
    self.s8.config(image=self.photo8,width="150",height="150")
    self.s8.grid(row=2,column=1)
    
    self.photo9=PhotoImage(file= image_path + "omg.gif")
    self.s9=Button(fsnd,command = lambda: self.play_sound("omg.wav"),justify = LEFT)
    self.s9.config(image=self.photo9,width="150",height="150")
    self.s9.grid(row=2,column=2)

    #list for other sounds
    flist = Frame(master,width = 300, height = 450,relief=RIDGE)
    flist.grid(row=1,column=1)
   
    flist_l = Frame(flist,width=300,height = 50)
    flist_l.grid(row=0,column=0)

    flist_list = Frame(flist,width=300,height = 250)
    flist_list.grid(row=1,column=0)

    self.l2 = Label(flist_l, text="Sounds List").pack()
    
    self.scrollbar = Scrollbar(flist_list)
    self.scrollbar.pack(side = RIGHT, fill=Y )
    self.mylist = Listbox(flist_list, yscrollcommand = self.scrollbar.set)
    
    #list all sounds
    allfiles = [f for f in os.listdir(sounds_path) if f.endswith('.wav')]

    for file in allfiles:
        self.mylist.insert(END, file)

    self.mylist.pack( side = LEFT, fill = BOTH )
    self.mylist.bind("<Double-Button-1>", self.OnSelection)
    self.scrollbar.config( command = self.mylist.yview )

  #control
  def rforward(self):
    sock.sendto('f', (UDP_IP, UDP_PORT)) 
  def rback(self):
    sock.sendto('b', (UDP_IP, UDP_PORT)) 
  def rleft(self):
    sock.sendto('l', (UDP_IP, UDP_PORT)) 
  def rright(self):
    sock.sendto('r', (UDP_IP, UDP_PORT)) 
  def rstop(self):
    sock.sendto('s', (UDP_IP, UDP_PORT)) 
  
  #send message
  def send_msg(self):
    print self.e1
    sock.sendto("Msg:"+self.e1.get(), (UDP_IP, UDP_PORT)) 

  #play sounds
  def play_sound(self,sound):
    pg.mixer.init()
    pg.mixer.music.load(sounds_path+sound)
    pg.mixer.music.play()

  #call back for list selected
  def OnSelection(self, event):
        widget = event.widget
        selection=widget.curselection()
        value = widget.get(selection[0])
        self.play_sound(value)
        print "selection:", selection, ": '%s'" % value

root = Tk()
root.title("Bibli v1.0")
root.geometry("750x550")
app = App(root)
root.mainloop()
