import subprocess

def compile_and_link():
    # Compile main.cpp into an object file
    subprocess.run(['g++', '-c', 'main.cpp', '-o', 'main.o'])

    # Link the object file with the event library
    subprocess.run(['g++', 'main.o', '-o', 'main', '-levent', '-lz'])

if __name__ == "__main__":
    compile_and_link()