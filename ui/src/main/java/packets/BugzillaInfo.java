package packets;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

public class BugzillaInfo implements Packet {
    public final String name;
    public final String url;
    public final String apiKey;
    public final String username;
    public String groupTasksBy = "";
    public int rootTaskID;
    public Map<String, String> labelToField = new HashMap<>();

    public BugzillaInfo(String name, String url, String apiKey, String username) {
        this.name = name;
        this.url = url;
        this.apiKey = apiKey;
        this.username = username;
    }

    public static BugzillaInfo parse(DataInputStream input) throws IOException {
        input.readInt(); // packet type
        String name = Packet.parseString(input);
        String url = Packet.parseString(input);
        String apiKey = Packet.parseString(input);
        String username = Packet.parseString(input);

        BugzillaInfo message = new BugzillaInfo(name, url, apiKey, username);

        message.rootTaskID = input.readInt();
        message.groupTasksBy = Packet.parseString(input);

        int labelToFieldCount = input.readInt();

        for (int i = 0; i < labelToFieldCount; i++) {
            String label = Packet.parseString(input);
            String field = Packet.parseString(input);

            message.labelToField.put(label, field);
        }

        return message;
    }

    @Override
    public void writeToOutput(DataOutputStream output) throws IOException {
        AtomicInteger size = new AtomicInteger(26 + name.length() + url.length() + apiKey.length() + username.length());
        size.addAndGet(groupTasksBy.length());
        labelToField.forEach((s, s2) -> {
            size.addAndGet(4);
            size.addAndGet(s.length());
            size.addAndGet(s2.length());
        });
        output.writeInt(size.get());
        output.writeInt(PacketType.BUGZILLA_INFO.value());
        Packet.writeString(output, name);
        Packet.writeString(output, url);
        Packet.writeString(output, apiKey);
        Packet.writeString(output, username);
        output.writeInt(rootTaskID);
        Packet.writeString(output, groupTasksBy);

        output.writeInt(labelToField.keySet().size());

        for (String label : labelToField.keySet()) {
            String field = labelToField.get(label);

            output.writeShort(label.length());
            output.write(label.getBytes());
            output.writeShort(field.length());
            output.write(field.getBytes());
        }
    }

    public String getGroupTasksBy() {
        return groupTasksBy;
    }

    public void setGroupTasksBy(String groupTasksBy) {
        this.groupTasksBy = groupTasksBy;
    }

    public int getRootTaskID() {
        return rootTaskID;
    }

    public void setRootTaskID(int rootTaskID) {
        this.rootTaskID = rootTaskID;
    }

    public Map<String, String> getLabelToField() {
        return labelToField;
    }

    public void setLabelToField(Map<String, String> labelToField) {
        this.labelToField = labelToField;
    }
}
