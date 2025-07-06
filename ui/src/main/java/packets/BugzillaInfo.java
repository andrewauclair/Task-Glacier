package packets;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

public class BugzillaInfo implements Packet {
    public final String name;
    public final String url;
    public final String apiKey;
    public final String username;
    public List<String> groupTasksBy = new ArrayList<>();
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

        int groupTasksByCount = input.readInt();

        for (int i = 0; i < groupTasksByCount; i++) {
            message.groupTasksBy.add(Packet.parseString(input));
        }

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
        AtomicInteger size = new AtomicInteger(30 + name.length() + url.length() + apiKey.length() + username.length());
        groupTasksBy.forEach(s -> {
            size.addAndGet(2);
            size.addAndGet(s.length());
        });
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

        output.writeInt(groupTasksBy.size());

        for (String groupBy : groupTasksBy) {
            Packet.writeString(output, groupBy);
        }

        output.writeInt(labelToField.keySet().size());

        for (String label : labelToField.keySet()) {
            String field = labelToField.get(label);

            output.writeShort(label.length());
            output.write(label.getBytes());
            output.writeShort(field.length());
            output.write(field.getBytes());
        }
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
