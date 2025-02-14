package packets;

import java.io.DataOutputStream;
import java.io.IOException;

public class BugzillaInfo implements Packet {
    private final String url;
    private final String apiKey;

    public BugzillaInfo(String url, String apiKey) {
        this.url = url;
        this.apiKey = apiKey;
    }

    @Override
    public void writeToOutput(DataOutputStream output) throws IOException {
        output.writeInt(12 + url.length() + apiKey.length());
        output.writeInt(PacketType.BUGZILLA_INFO.value());
        output.writeShort((short) url.length());
        output.write(url.getBytes());
        output.writeShort((short) apiKey.length());
        output.write(apiKey.getBytes());
    }
}
