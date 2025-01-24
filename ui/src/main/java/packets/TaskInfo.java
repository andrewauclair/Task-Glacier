package packets;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class TaskInfo {
    public int taskID = 0;
    public int parentID = 0;
    public String name = "";

    public static TaskInfo parse(DataInputStream input) throws IOException {
        TaskInfo info = new TaskInfo();

        input.readInt(); // packet type
        info.taskID = input.readInt(); // task id
        info.parentID = input.readInt(); // parent id

        int chars = input.readShort(); // string length

        byte[] bytes = input.readNBytes(chars);

        info.name = new String(bytes);

        return info;
    }

    public void write(DataOutputStream output) throws IOException {
        output.writeInt(PacketType.TASK_INFO.value());
        output.writeInt(taskID);
        output.writeInt(parentID);
        output.writeShort(name.length());
        output.write(name.getBytes());
    }
}
