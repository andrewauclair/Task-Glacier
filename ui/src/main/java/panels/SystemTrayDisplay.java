package panels;

import taskglacier.MainFrame;

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.geom.RoundRectangle2D;
import java.util.Objects;

public class SystemTrayDisplay extends JFrame {
    private final TrayIcon trayIcon;
    private final TaskSearch search;

    // https://www.flaticon.com/free-icon/cognitive_8920590?term=cognitive&related_id=8920590
    // https://www.flaticon.com/free-icon/clipboard_1527478?term=task&page=1&position=53&origin=search&related_id=1527478
    // https://www.flaticon.com/free-icon/share_8584964?term=open&page=1&position=3&origin=search&related_id=8584964

    JButton openApp = new JButton(new ImageIcon(Objects.requireNonNull(getClass().getResource("/share.png"))));
    JButton unspecifiedTask = new JButton(new ImageIcon(Objects.requireNonNull(getClass().getResource("/cognitive.png"))));
    JButton dailyReport = new JButton(new ImageIcon(Objects.requireNonNull(getClass().getResource("/clipboard.png"))));

    Timer timer;
    MouseEvent e;
    MouseAdapter listener = new MouseAdapter() {
        @Override
        public void mouseClicked(MouseEvent e) {
            if (isVisible()) {
                return;
            }
            SystemTrayDisplay.this.e = e;

            if (e.getClickCount() > 1) {
                MainFrame.mainFrame.setVisible(true);
                timer.stop();
                return;
            }

            if (!timer.isRunning()) {
                timer.start();
            }
        }

        @Override
        public void mouseEntered(MouseEvent e) {
            System.out.println("Mouse entered");
        }
    };

    public SystemTrayDisplay(MainFrame mainFrame, TrayIcon trayIcon) {
        this.trayIcon = trayIcon;
        search = new TaskSearch(mainFrame);

        Timer test = new Timer(0, e2 -> {
            trayIcon.setToolTip("Multi\nline\ntool\ntip\n" + String.valueOf(System.currentTimeMillis()));
        });
        test.setDelay(1000);
        test.setRepeats(true);
        test.start();
        timer = new Timer(250, e1 -> {
            if (isVisible()) {
                // hide
                setVisible(!isVisible());
            }
            else if (e != null) {
                double scale = GraphicsEnvironment
                        .getLocalGraphicsEnvironment()
                        .getDefaultScreenDevice() // or cycle your getScreenDevices()
                        .getDefaultConfiguration()
                        .getDefaultTransform()
                        .getScaleX();

                search.updateTasks();

                setVisible(true);

                Rectangle bounds = GraphicsEnvironment.getLocalGraphicsEnvironment()
                        .getDefaultScreenDevice()
                        .getDefaultConfiguration()
                        .getBounds();

                Point p = new Point(bounds.width - getWidth() - 10, (int) ((e.getLocationOnScreen().y / scale) - getHeight() - 40));

                setLocation(p);
            }
        });
        timer.setRepeats(false);
        timer.start();

        trayIcon.addMouseListener(listener);

        setUndecorated(true);
        setShape(new RoundRectangle2D.Double(0, 0, getWidth(), getHeight(), 25, 25));

        addWindowFocusListener(new WindowAdapter() {
            @Override
            public void windowLostFocus(WindowEvent e) {
                if (!timer.isRunning()) {
                    timer.restart();
                }
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
        gbc.insets = new Insets(5, 5, 5, 5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        add(search, gbc);

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
