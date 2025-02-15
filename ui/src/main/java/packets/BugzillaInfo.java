package packets;

import java.io.DataOutputStream;
import java.io.IOException;

public class BugzillaInfo implements Packet {
    private final String url;
    private final String apiKey;
    private final String username;

    public BugzillaInfo(String url, String apiKey, String username) {
        this.url = url;
        this.apiKey = apiKey;
        this.username = username;
    }

    @Override
    public void writeToOutput(DataOutputStream output) throws IOException {
        output.writeInt(12 + url.length() + apiKey.length() + username.length());
        output.writeInt(PacketType.BUGZILLA_INFO.value());
        output.writeShort((short) url.length());
        output.write(url.getBytes());
        output.writeShort((short) apiKey.length());
        output.write(apiKey.getBytes());
        output.writeShort((short) username.length());
        output.write(username.getBytes());
    }
}
