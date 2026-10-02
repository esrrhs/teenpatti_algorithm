// Command teenpatti_gen regenerates teenpatti_data.txt into the working
// directory. It is the Go counterpart of running the Java
// TeenPattiAlgorithmUtil.main: GenUtil.genKey() followed by
// GenUtil.outputData().
package main

import (
	"log"

	teenpatti "github.com/esrrhs/teenpatti_algorithm/go"
)

func main() {
	teenpatti.GenKey()
	if err := teenpatti.OutputData(); err != nil {
		log.Fatal(err)
	}
}
