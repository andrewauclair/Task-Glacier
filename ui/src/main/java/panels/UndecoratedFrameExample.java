import javax.swing.*;
import java.awt.*;

public class UndecoratedFrameExample {
    public static void main(String[] args) {
        // Create the frame
        JFrame frame = new JFrame("Undecorated JFrame Example");

        // Set the frame to be undecorated
        frame.setUndecorated(true);

        // Set the layout manager
        frame.setLayout(new BorderLayout());

        // Add some components
//        frame.add(new JButton("North"), BorderLayout.NORTH);
//        frame.add(new JButton("South"), BorderLayout.SOUTH);
//        frame.add(new JButton("East"), BorderLayout.EAST);
//        frame.add(new JButton("West"), BorderLayout.WEST);
        frame.add(new JButton("Center"), BorderLayout.CENTER);

        // Set default close operation
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

        // Set the frame size
        frame.setSize(400, 300);

        // Center the frame on the screen
        frame.setLocationRelativeTo(null);

        // Make the frame visible
        frame.setVisible(true);
    }
}