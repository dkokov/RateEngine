import java.net.*;
import java.io.*;

public class my_cc {
    public static void main(String[] args) {
	String serverName = args[0];
	int port = Integer.parseInt(args[1]);
	
	try{
	    System.out.println("Connecting to " + serverName + " on port " + port);
	
	    Socket client = new Socket(serverName, port);
	    System.out.println("Just connected to " + client.getRemoteSocketAddress());
	
	    OutputStream outToServer = client.getOutputStream();
	    DataOutputStream out = new DataOutputStream(outToServer);
	    //out.writeUTF("Hello from " + client.getLocalSocketAddress());
	    out.writeBytes("42,1,maxsec,1234,35924119460,359884119998,1111");
	
	    //InputStream inFromServer = client.getInputStream();
	    //DataInputStream in = new DataInputStream(inFromServer);
	
	    BufferedReader inFromServer = new BufferedReader(new InputStreamReader(client.getInputStream()));
	    System.out.println("RateEngine: " + inFromServer.readLine());
	
	    client.close();
	} catch(IOException e) {
	    e.printStackTrace();
	}
   }
}
