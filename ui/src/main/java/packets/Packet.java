package packets;

import java.io.DataOutputStream;
import java.io.IOException;

public interface Packet {
    void writeToOutput(DataOutputStream output) throws IOException;
}
