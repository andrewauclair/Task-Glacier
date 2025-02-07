package panels;

import taskglacier.MainFrame;

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.geom.RoundRectangle2D;
import java.util.Objects;

public class SystemTrayDisplay extends JFrame {
    private final TrayIcon trayIcon;
    JTextField search = new JTextField();


    // https://www.flaticon.com/free-icon/cognitive_8920590?term=cognitive&related_id=8920590
    // https://www.flaticon.com/free-icon/clipboard_1527478?term=task&page=1&position=53&origin=search&related_id=1527478
    // https://www.flaticon.com/free-icon/share_8584964?term=open&page=1&position=3&origin=search&related_id=8584964

    JButton openApp = new JButton(new ImageIcon(Objects.requireNonNull(getClass().getResource("/share.png"))));
    JButton unspecifiedTask = new JButton(new ImageIcon(Objects.requireNonNull(getClass().getResource("/cognitive.png"))));
    JButton dailyReport = new JButton(new ImageIcon(Objects.requireNonNull(getClass().getResource("/clipboard.png"))));

    MouseAdapter listener = new MouseAdapter() {
        @Override
        public void mouseReleased(MouseEvent e) {
            double scale = GraphicsEnvironment
                    .getLocalGraphicsEnvironment()
                    .getDefaultScreenDevice() // or cycle your getScreenDevices()
                    .getDefaultConfiguration()
                    .getDefaultTransform()
                    .getScaleX();

            setVisible(true);

            trayIcon.removeMouseListener(listener);

            Rectangle bounds = GraphicsEnvironment.getLocalGraphicsEnvironment()
                    .getDefaultScreenDevice()
                    .getDefaultConfiguration()
                    .getBounds();

            Point p = new Point(bounds.width - getWidth() - 10, (int) ((e.getLocationOnScreen().y / scale) - getHeight() - 40));

            setLocation(p);
        }
    };

    public SystemTrayDisplay(TrayIcon trayIcon) {
        this.trayIcon = trayIcon;

        Timer timer = new Timer(500, e -> SwingUtilities.invokeLater(() -> trayIcon.addMouseListener(listener)));
        timer.start();

        setUndecorated(true);
        setShape(new RoundRectangle2D.Double(0, 0, getWidth(), getHeight(), 25, 25));

        addWindowListener(new WindowAdapter() {
            @Override
            public void windowDeactivated(WindowEvent e) {
                setVisible(false);
                trayIcon.removeMouseListener(listener);
                timer.restart();
            }
        });

        PopupMenu popup = new PopupMenu();
        MenuItem exit = new MenuItem("Exit");

        exit.addActionListener(e -> System.exit(0));

        popup.add(exit);

        trayIcon.setPopupMenu(popup);

        // the frame is undecorated, so we have to do it all ourselves
        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.gridwidth = 1;
        gbc.gridheight = 1;
        gbc.weightx = 0;
        gbc.weighty = 0;
        gbc.fill = GridBagConstraints.NONE;

        add(search, gbc);

        gbc.gridy++;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        add(new JLabel(), gbc);

        gbc.gridy++;
        gbc.weightx = 0;
        gbc.weighty = 0;
        gbc.fill = GridBagConstraints.NONE;

        add(createButtonsPanel(), gbc);

        setSize(400, 600);
    }

    private JPanel createButtonsPanel() {
        JPanel panel = new JPanel(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.gridheight = 1;
        gbc.gridwidth = 1;
        gbc.weightx = 0;
        gbc.weighty = 0;
        gbc.fill = GridBagConstraints.NONE;

        openApp.addActionListener(e -> MainFrame.mainFrame.setVisible(true));

        panel.add(dailyReport, gbc);
        gbc.gridx++;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.weightx = 1;

        panel.add(unspecifiedTask, gbc);
        gbc.gridx++;
        gbc.fill = GridBagConstraints.NONE;
        gbc.weightx = 0;

        panel.add(openApp, gbc);

        return panel;
    }
}
