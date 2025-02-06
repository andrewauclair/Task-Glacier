package panels;

import javax.swing.*;
import java.awt.*;
import java.awt.geom.RoundRectangle2D;

public class SystemTrayDisplay extends JFrame {
    JTextField search = new JTextField();



    JButton openApp = new JButton();
    JButton unspecifiedTask = new JButton();
    JButton dailyReport = new JButton();

    public SystemTrayDisplay() {
        setSize(400, 600);
        setUndecorated(true);
        setShape(new RoundRectangle2D.Double(0, 0, getWidth(), getHeight(), 25, 25));

        // the frame is undecorated, so we have to do it all ourselves
        setLayout(null);

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.fill = GridBagConstraints.NONE;

        add(search, gbc);
        gbc.gridy++;
        gbc.fill = GridBagConstraints.BOTH;

        add(new JLabel(), gbc);
        gbc.gridy++;
        gbc.fill = GridBagConstraints.NONE;

        add(createButtonsPanel(), gbc);

//        pack();
        // display name of server instance. not implemented in the server yet

        // display task favorites. not implemented yet

        // display list of tasks, most recently first. active on top, if there is an active task

        // button to display the full app
        // button to start an unknown task. not implemented yet. this is a feature that lets you
        //  quickly switch tasks without finding the right one to start first
        // button to perform a search
    }
    private JPanel createButtonsPanel() {
        JPanel panel = new JPanel(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.fill = GridBagConstraints.NONE;

        panel.add(openApp, gbc);
        gbc.gridx++;
        gbc.fill = GridBagConstraints.HORIZONTAL;

        panel.add(unspecifiedTask, gbc);
        gbc.gridx++;
        gbc.fill = GridBagConstraints.HORIZONTAL;

        panel.add(dailyReport, gbc);

        return panel;
    }
}
