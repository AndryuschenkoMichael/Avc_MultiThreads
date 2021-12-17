package main

import (
	"fmt"
	"sync"
	"time"
)

var (
	dataBase      = make(map[string]string)
	mutexDataBase = make(chan struct{}, 1)
)

var (
	countReader = 0
	mutexReader = make(chan struct{}, 1)
)

type Reader struct {
	Name string
}

func (r *Reader) Read(key string) {
	mutexReader <- struct{}{}
	countReader++
	if countReader == 1 {
		mutexDataBase <- struct{}{}
	}
	<-mutexReader

	// Read data from map
	fmt.Printf("Hello, I'm %s. I read from data base\n", r.Name)

	mutexReader <- struct{}{}
	countReader--
	if countReader == 0 {
		<-mutexDataBase
	}
	<-mutexReader

	time.Sleep(200)
}

type Writer struct {
	Name string
}

func (w *Writer) Write(key, value string) {
	mutexDataBase <- struct{}{}

	// Write data to database
	fmt.Printf("Hello, I'm %s. I write into data base\n", w.Name)

	<-mutexDataBase

	time.Sleep(400)
}

func main() {
	wg := sync.WaitGroup{}
	for i := 0; i < 4; i++ {
		wg.Add(1)
		reader := Reader{Name: "Reader" + fmt.Sprintf("Reader %d", i)}
		go func() {
			reader.Read("Some name")
			wg.Done()
		}()
	}

	for i := 0; i < 12; i++ {
		wg.Add(1)
		writer := Writer{Name: fmt.Sprintf("Writer %d", i)}
		go func() {
			writer.Write("Some key", "Some value")
			wg.Done()
		}()
	}

	wg.Wait()
}
