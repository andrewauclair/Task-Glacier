package dialogs;

import packets.PacketType;
import packets.RequestID;
import packets.TaskInfo;
import packets.UpdateTaskTimes;
import raven.datetime.DatePicker;
import raven.datetime.TimePicker;
import taskglacier.MainFrame;
import util.LabeledComponent;

import javax.swing.*;
import java.awt.*;
import java.time.Instant;
import java.time.ZoneId;
import java.time.ZonedDateTime;
import java.util.Optional;

public class SessionEdit extends JDialog {
    public static SessionEdit openInstance = null;
    public static int requestID = 0;

    public TaskInfo.Session session = null;
    private int sessionIndex = 0;

    private DatePicker startDatePicker = new DatePicker();
    private DatePicker stopDatePicker = new DatePicker();

    private TimePicker startTimePicker = new TimePicker();
    private TimePicker stopTimePicker = new TimePicker();

    JFormattedTextField startDate = new JFormattedTextField();
    JFormattedTextField stopDate = new JFormattedTextField();

    JFormattedTextField startTime = new JFormattedTextField();
    JFormattedTextField stopTime = new JFormattedTextField();

    JCheckBox stopPresent = new JCheckBox("Include Stop");

    JButton save = new JButton("Save");
    boolean newSession = true;
    private int taskID;

    public SessionEdit(MainFrame mainFrame, int taskID) {
        super(mainFrame);
        this.taskID = taskID;

        setModalityType(ModalityType.APPLICATION_MODAL);

        setTitle("Add Session");

        startDatePicker.setDateFormat("MM/dd/yyyy");
        stopDatePicker.setDateFormat("MM/dd/yyyy");

        startDate.setText("--/--/----");
        stopDate.setText("--/--/----");

        startDatePicker.setEditor(startDate);
        stopDatePicker.setEditor(stopDate);

        startTime.setText("--:--:-- --");
        stopTime.setText("--:--:-- --");


        startTimePicker.setEditor(startTime);
        stopTimePicker.setEditor(stopTime);

        JPanel start = new JPanel();
        start.setBorder(BorderFactory.createTitledBorder("Start"));

        start.add(new LabeledComponent("Date", startDate));
        start.add(new LabeledComponent("Time", startTime));

        JPanel stop = new JPanel();
        stop.setBorder(BorderFactory.createTitledBorder("Stop"));

        stop.add(new LabeledComponent("Date", stopDate));
        stop.add(new LabeledComponent("Time", stopTime));

        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.insets = new Insets(5, 5, 5, 5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 0;
        gbc.weighty = 0;

        gbc.gridx++;

        add(stopPresent, gbc);
        gbc.gridx = 0;
        gbc.gridy++;

        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.HORIZONTAL;

        add(createStartPanel(), gbc);
        gbc.gridx++;

        add(createStopPanel(), gbc);
        gbc.gridy++;

        save.setEnabled(false);

        save.addActionListener(e -> {
            // send updated session to server
            PacketType type = session != null ? PacketType.EDIT_TASK_SESSION : PacketType.ADD_TASK_SESSION;

            startDatePicker.getSelectedDate();
            startTimePicker.getSelectedTime();

            ZonedDateTime startDate = ZonedDateTime.of(startDatePicker.getSelectedDate(), startTimePicker.getSelectedTime(), ZoneId.systemDefault());
            ZonedDateTime stopDate = ZonedDateTime.of(stopDatePicker.getSelectedDate(), stopTimePicker.getSelectedTime(), ZoneId.systemDefault());

            Instant startTime = startDate.toInstant();
            Optional<Instant> stopTime = Optional.of(stopDate.toInstant());

            if (!stopPresent.isSelected()) {
                stopTime = Optional.empty();
            }

            requestID = RequestID.nextRequestID();
            openInstance = this;

            UpdateTaskTimes packet = new UpdateTaskTimes(type, requestID, taskID, sessionIndex, startTime, stopTime);
            mainFrame.getConnection().sendPacket(packet);
        });

        startDate.addPropertyChangeListener(e -> updateSave());
        startTime.addPropertyChangeListener(e -> updateSave());
        stopPresent.addActionListener(e -> updateSave());
        stopDate.addPropertyChangeListener(e -> updateSave());
        stopTime.addPropertyChangeListener(e -> updateSave());

        gbc.weightx = 0;
        gbc.weighty = 0;

        gbc.anchor = GridBagConstraints.SOUTHEAST;
        gbc.fill = GridBagConstraints.NONE;

        add(save, gbc);

        setSize(400, 200);

        // center on the main frame
        setLocationRelativeTo(mainFrame);

        stopPresent.addActionListener(e -> {
            stopDate.setEnabled(stopPresent.isSelected());
            stopTime.setEnabled(stopPresent.isSelected());
        });

        stopDate.setEnabled(false);
        stopTime.setEnabled(false);
    }

    public SessionEdit(MainFrame mainFrame, int taskID, TaskInfo.Session session, int sessionIndex) {
        this(mainFrame, taskID);

        setTitle("Edit Session");

        this.session = session;
        this.sessionIndex = sessionIndex;
        newSession = false;

        ZonedDateTime startDateTime = session.startTime.atZone(ZoneId.systemDefault());
        startDatePicker.setSelectedDate(startDateTime.toLocalDate());
        startTimePicker.setSelectedTime(startDateTime.toLocalTime());

        stopPresent.setSelected(session.stopTime.isPresent());

        if (session.stopTime.isPresent()) {
            stopDate.setEnabled(true);
            stopTime.setEnabled(true);

            ZonedDateTime stopDateTime = session.stopTime.get().atZone(ZoneId.systemDefault());
            stopDatePicker.setSelectedDate(stopDateTime.toLocalDate());
            stopTimePicker.setSelectedTime(stopDateTime.toLocalTime());
        }
    }

    public void close() {
        openInstance = null;
        requestID = 0;
        dispose();
    }

    private JPanel createStartPanel() {
        JPanel panel = new JPanel(new GridBagLayout());
        panel.setBorder(BorderFactory.createTitledBorder("Start"));

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.insets = new Insets(5, 5, 5, 5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.weightx = 1;
        gbc.weighty = 0;

        panel.add(new LabeledComponent("Date", startDate), gbc);
        gbc.gridy++;

        panel.add(new LabeledComponent("Time", startTime), gbc);
        gbc.gridy++;

        return panel;
    }

    private JPanel createStopPanel() {
        JPanel panel = new JPanel(new GridBagLayout());
        panel.setBorder(BorderFactory.createTitledBorder("Stop"));

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.insets = new Insets(5, 5, 5, 5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.weightx = 1;
        gbc.weighty = 0;

        panel.add(new LabeledComponent("Date", stopDate), gbc);
        gbc.gridy++;

        panel.add(new LabeledComponent("Time", stopTime), gbc);
        gbc.gridy++;

        return panel;
    }

    private void updateSave() {
        boolean startValid = startDatePicker.isDateSelected() && startTimePicker.isTimeSelected();
        boolean stopValid = !stopPresent.isSelected() || (stopDatePicker.isDateSelected() && stopTimePicker.isTimeSelected());

        save.setEnabled(startValid && stopValid);
    }
}
