import java.io.*;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.time.OffsetDateTime;
import java.util.Arrays;
import java.util.Scanner;

import org.json.JSONObject;

public class Main {
    private static void sendJSON(DataOutputStream output, JSONObject object) throws IOException {
        String string = object.toString();

        short size = (short) (2 + string.length());

        output.write(ByteBuffer.allocate(2).putShort(size).array());
        output.write(string.getBytes());

        output.flush();
    }

    public static void main(String[] args) throws IOException {
        System.out.println("Connect to micro-task C++ CLI");

        final Socket socket = new Socket("127.0.0.1", 5005);

        System.out.println("Connected");

        DataOutputStream output = new DataOutputStream(socket.getOutputStream());

        JSONObject versionRequest = new JSONObject();
        versionRequest.put("command", 1);

        sendJSON(output, versionRequest);

        Thread listen = new Thread(() -> {
        try (DataInputStream in = new DataInputStream(socket.getInputStream())) {
            int packetLength;
            while ((packetLength = in.readShort()) != -1) {
                int expectedBytes = packetLength - 2;

                byte[] bytes = new byte[expectedBytes];

                int totalRead = 0;

                while (totalRead < expectedBytes) {
                    int read = in.read(bytes, totalRead, bytes.length - totalRead);
                    if (read == -1) {
                        totalRead = -1;
                        break;
                    }
                    totalRead += read;
                }

                if (totalRead == -1) {
                    break;
                }

                String source = new String(Arrays.copyOfRange(bytes, 0, bytes.length));
                JSONObject obj = new JSONObject(source);

                switch (obj.getInt("command")) {
                    case 1: // version request response
                        System.out.println("Version: " + obj.getString("version"));
                        break;
                    case 2: // add task
                        // this response verifies that the task was added
                        break;
                    case 3: // start task
                        break;
                    case 4: // get task response
                        OffsetDateTime addTime = OffsetDateTime.parse(obj.getString("add-time"));
                        System.out.printf("Task %d %s %s%n", obj.getInt("id"), addTime, obj.getString("name"));

                        if (obj.has("start-time")) {
                            OffsetDateTime startTime = OffsetDateTime.parse(obj.getString("start-time"));

                            System.out.println("start time: " + startTime);
                        }
                        break;
                }
            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        });

        listen.start();

        Scanner input = new Scanner(System.in);

        while (input.hasNextLine()) {
            String line = input.nextLine();

            if (line.startsWith("add")) {
                String name = line.substring(4);

                JSONObject addTask = new JSONObject();
                addTask.put("command", 2);
                addTask.put("name", name);

                sendJSON(output, addTask);
            }
            else if (line.startsWith("start")) {
                int id = Integer.parseInt(line.substring(6));

                JSONObject startTask = new JSONObject();
                startTask.put("command", 3);
                startTask.put("id", id);

                sendJSON(output, startTask);
            }
            else if (line.startsWith("get")) {
                int id = Integer.parseInt(line.substring(4));

                JSONObject getTask = new JSONObject();
                getTask.put("command", 4);
                getTask.put("id", id);

                sendJSON(output, getTask);
            }
        }
    }
}
