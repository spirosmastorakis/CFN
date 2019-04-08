import pickle
import dill
import hashlib

def createStateInfo(cls):
    print("Pickled", hashlib.md5(dill.dumps(cls)).hexdigest())



def time_this(original_function):
    print("decorating")
    def new_function(*args,**kwargs):
        print("starting timer")
        import datetime
        before = datetime.datetime.now()
        x = original_function(*args,**kwargs)
        after = datetime.datetime.now()
        print("Elapsed Time = {0}".format(after-before))
        return x
    return new_function

def time_all_class_methods(Cls):
    class NewCls(object):
        def __init__(self,*args,**kwargs):
            self.oInstance = Cls(*args,**kwargs)
            print("New class name", type(self.oInstance).__name__)

        def __getattribute__(self,s):
            """
            this is called whenever any attribute of a NewCls object is accessed. This function first tries to
            get the attribute off NewCls. If it fails then it tries to fetch the attribute from self.oInstance (an
            instance of the decorated class). If it manages to fetch the attribute from self.oInstance, and
            the attribute is an instance method then `time_this` is applied.
            """
            try:
                x = super(NewCls,self).__getattribute__(s)
            except AttributeError:
                pass
            else:
                return x
            x = self.oInstance.__getattribute__(s)

            if type(x) == type(self.__init__): # it is an instance method
                print(type(x.__self__).__name__)
                #print("Pickled", hashlib.md5(dill.dumps(x)).hexdigest())
                createStateInfo(x)
                return time_this(x)                 # this is equivalent of just decorating the method with time_this
            else:
                return x
    NewCls.__name__ = Cls.__name__
    return NewCls

#now lets make a dummy class to test it out on:

@time_all_class_methods
class Foo(object):
    def __init__(self):
        pass

    def a(self):
        print("entering a")
        print("exiting a")

oF = Foo()
oF.a()
