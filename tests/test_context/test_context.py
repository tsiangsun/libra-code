import os
import sys
import math

# Fisrt, we add the location of the library to test to the PYTHON path
cwd = os.getcwd()
print "Current working directory", cwd
sys.path.insert(1,cwd+"/../../_build/src/mmath")
sys.path.insert(1,cwd+"/../../_build/src/context")


print "\nTest 1: Importing the library and its content"
from cygmmath import *
from cygcontext import *

print "\nTest 2: Building an empty context"
ctx = Context()
print "path=", ctx.get_path()

print "\nTest 3: We can \"view\" the context object by printing it as an XML file"
ctx.save_xml("ctx_1.xml")
print "path=", ctx.get_path()


print "\nTest 4: The easiest way to build some non-empty context is to read it from an XML file"
print "\nUsing constructor"
ctx = Context("ctx_example.xml")
print "path=", ctx.get_path()
ctx.save_xml("ctx_2.xml")


print "\nTest 4a: Read XML, change its path, and print it again"
print "\nUsing constructor"
ctx = Context("ctx_example.xml")
print "path=", ctx.get_path()
ctx.set_path("new_control_params")
print "path=", ctx.get_path()
ctx.save_xml("ctx_2a.xml")



print "\nTest 5: Now lets see how one can add new variables to empty context"
ctx = Context()
ctx.set_path("new_path")
print "path=", ctx.get_path()

print "add integer-valued variable"
param1 = 1.0
ctx.add("param1", param1)
print ctx.get("param1", -1) 
print ctx.get("param1a", -1) 

print "add string-valued variable"
param2 = "Chalk"
ctx.add("param2", param2)
print ctx.get("param2", "Milk") 
print ctx.get("param2a", "Milk") 

print "add integer-list-valued variable"
param3 = intList()
for i in xrange(3):
    param3.append(i)
ctx.add("param3", param3)

d = intList()
d.append(-1)
print ctx.get("param3", d)[0],ctx.get("param3", d)[1],ctx.get("param3", d)[2]
print ctx.get("param3a", d)[0]


print "printing the resulting structure in ctx_3.xml"
ctx.save_xml("ctx_3.xml")


print "\nTest 6: We can also add one context into another one"
ctx1 = Context("ctx_3.xml")
ctx1.set_path("old_path")

p1 = 2.0
ctx.add("param1", p1)
ctx.save_xml("ctx_4a.xml")

ctx.add(ctx1) 
ctx.save_xml("ctx_4b.xml")



print "\nTest 7: We can extract one context from the other"
ctx2 = ctx.get("old_path", ctx)
print "path=", ctx2.get_path()
ctx2.save_xml("ctx_5.xml") 



