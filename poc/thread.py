import threading
import time

def operator(vector:list):
    for i in vector:
        values = i**i
        time.sleep(1)
        print(f"the i2 of {i} is {values}")

def print_char(texto:str):
    for i in texto:
        time.sleep(1)
        print(f"the char vector is {i}")        


vector = [1,2,3]
texto = "FIAP"

t_operator = threading.Thread(target=operator, args=(vector,))
t_printchar = threading.Thread(target=print_char, args=(texto,))

t_operator.start()
t_printchar.start()

t_operator.join()
t_printchar.join()