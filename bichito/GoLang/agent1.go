package main

import (
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
	"path/filepath"
	"syscall"
	"time"
)

func main() {
	// Ruta del archivo en el escritorio
	desktopPath, _ := os.UserHomeDir()
	desktopFilePath := filepath.Join(desktopPath, "test_file.txt")

	// Crear un archivo en el escritorio
	err := ioutil.WriteFile(desktopFilePath, []byte("Contenido del archivo"), 0644)
	if err != nil {
		fmt.Println("Error al escribir el archivo en el escritorio:", err)
		return
	}
	fmt.Println("Archivo creado en el escritorio:", desktopFilePath)

	// Ejecutar calc.exe en segundo plano
	cmd := exec.Command("calc.exe")

	// Configurar el atributo SysProcAttr para desvincular el proceso secundario del proceso principal
	cmd.SysProcAttr = &syscall.SysProcAttr{CreationFlags: syscall.CREATE_NEW_PROCESS_GROUP}

	err = cmd.Start()
	if err != nil {
		fmt.Println("Error al iniciar calc.exe:", err)
		return
	}

	// Esperar unos segundos antes de finalizar el programa principal
	time.Sleep(5 * time.Second)
	fmt.Println("Programa principal finalizado")
}
