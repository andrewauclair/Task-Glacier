package panels;

import javax.swing.*;
import java.awt.event.FocusEvent;
import java.awt.event.FocusListener;
import java.awt.geom.RoundRectangle2D;

public class SystemTrayDisplay extends JFrame {
    public SystemTrayDisplay() {
        setSize(400, 600);
        setUndecorated(true);
        setShape(new RoundRectangle2D.Double(0, 0, getWidth(), getHeight(), 25, 25));

        // display name of server instance. not implemented in the server yet

        // display task favorites. not implemented yet

        // display list of tasks, most recently first. active on top, if there is an active task

        // button to display the full app
        // button to start an unknown task. not implemented yet. this is a feature that lets you
        //  quickly switch tasks without finding the right one to start first
        // button to perform a search
    }
}
