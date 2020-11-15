package main

import (
	"encoding/binary"
	"fmt"
	"golang.org/x/text/encoding/simplifiedchinese"
	"log"
	"net"
)
import "github.com/axgle/mahonia"

func ConvertGBK2Str(gbkStr []byte) string {
	b, _ := simplifiedchinese.GBK.NewDecoder().Bytes(gbkStr)
	return string(b)
}

func Convert2GBK(str string) string {

	return mahonia.NewEncoder("gbk").ConvertString(str)

}

func startServer() {

	address := net.TCPAddr{
		IP:   net.ParseIP("127.0.0.1"), // 把字符串IP地址转换为net.IP类型
		Port: 8777,
	}
	listener, err := net.ListenTCP("tcp4", &address) // 创建TCP4服务器端监听器
	if err != nil {
		log.Fatal(err) // Println + os.Exit(1)
	}
	for {
		conn, err := listener.AcceptTCP()
		if err != nil {
			log.Fatal(err) // 错误直接退出
		}

		var wxIdLen uint32
		binary.Read(conn, binary.BigEndian, &wxIdLen)
		wxId := make([]byte, wxIdLen)
		conn.Read(wxId)
		strWxId := ConvertGBK2Str(wxId)

		var wxMsgLen uint32
		binary.Read(conn, binary.BigEndian, &wxMsgLen)
		wxMsg := make([]byte, wxMsgLen)
		conn.Read(wxMsg)
		strWxMsg := ConvertGBK2Str(wxMsg)
		recvTxTMessageCallBack(strWxId, strWxMsg)

	}
}

func sendTxTMessage(wXid string, wXms string) {
	var wXlen = make([]byte, 4)
	var mSlen = make([]byte, 4)
	fmt.Println(len(Convert2GBK(wXms)))
	binary.BigEndian.PutUint32(wXlen, uint32(len(Convert2GBK(wXid))))
	binary.BigEndian.PutUint32(mSlen, uint32(len(Convert2GBK(wXms))))

	conn, _ := net.Dial("tcp", "127.0.0.1:27015")
	conn.Write(wXlen)
	conn.Write([]byte(Convert2GBK(wXid)))

	conn.Write(mSlen)
	conn.Write([]byte(Convert2GBK(wXms)))

	conn.Close()
}

func recvTxTMessageCallBack(wXid string, wXms string) {
	sendTxTMessage(wXid,wXms);
}



func main() {
	startServer()
}
