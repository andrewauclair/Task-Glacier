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
import java.util.Collections;
import java.util.Optional;

public class SessionEdit extends JDialog {
    public static SessionEdit openInstance = null;
    public static int requestID = 0;

    public TaskInfo.Session session = null;

    private DatePicker startDatePicker = new DatePicker();
    private DatePicker stopDatePicker = new DatePicker();

    private TimePicker startTimePicker = new TimePicker();
    private TimePicker stopTimePicker = new TimePicker();

    JFormattedTextField startDate = new JFormattedTextField();
    JFormattedTextField stopDate = new JFormattedTextField();

    JFormattedTextField startTime = new JFormattedTextField();
    JFormattedTextField stopTime = new JFormattedTextField();

    JCheckBox stopPresent = new JCheckBox("Include Stop");

    JButton ok = new JButton("OK");
    JButton cancel = new JButton("Cancel");
    JButton apply = new JButton("Apply");

    boolean newSession = true;
    int sessionIndex = 0;

    public SessionEdit(MainFrame mainFrame, int taskID) {
        super(mainFrame);

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

        ok.addActionListener(e -> {
            apply.doClick();
            cancel.doClick();
        });

        cancel.addActionListener(e -> dispose());

        apply.setEnabled(false);

        apply.addActionListener(e -> {
            ZonedDateTime startDate = ZonedDateTime.of(startDatePicker.getSelectedDate(), startTimePicker.getSelectedTime(), ZoneId.systemDefault());

            Instant startTime = startDate.toInstant();
            Optional<Instant> stopTime = Optional.empty();

            if (stopPresent.isSelected()) {
                ZonedDateTime stopDate = ZonedDateTime.of(stopDatePicker.getSelectedDate(), stopTimePicker.getSelectedTime(), ZoneId.systemDefault());

                stopTime = Optional.of(stopDate.toInstant());
            }

            requestID = RequestID.nextRequestID();
            openInstance = this;

            UpdateTaskTimes packet = new UpdateTaskTimes(newSession ? PacketType.ADD_TASK_SESSION : PacketType.EDIT_TASK_SESSION, requestID, taskID, sessionIndex, startTime,
                    stopTime);
            packet.checkForOverlap = true;
            mainFrame.getConnection().sendPacket(packet);
        });

        startDate.addPropertyChangeListener(e -> updateApply());
        startTime.addPropertyChangeListener(e -> updateApply());
        stopPresent.addActionListener(e -> updateApply());
        stopDate.addPropertyChangeListener(e -> updateApply());
        stopTime.addPropertyChangeListener(e -> updateApply());

        gbc.weightx = 0;
        gbc.weighty = 0;

        gbc.anchor = GridBagConstraints.SOUTHEAST;
        gbc.fill = GridBagConstraints.NONE;
        gbc.gridx = 0;
        gbc.gridwidth = 2;

        add(createButtonPanel(), gbc);

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

    public void successResponse() {
        requestID = 0;

        apply.setEnabled(false);

        ZonedDateTime startDate = ZonedDateTime.of(startDatePicker.getSelectedDate(), startTimePicker.getSelectedTime(), ZoneId.systemDefault());

        Instant startTime = startDate.toInstant();
        Optional<Instant> stopTime = Optional.empty();

        if (stopPresent.isSelected()) {
            ZonedDateTime stopDate = ZonedDateTime.of(stopDatePicker.getSelectedDate(), stopTimePicker.getSelectedTime(), ZoneId.systemDefault());

            stopTime = Optional.of(stopDate.toInstant());
        }

        session = new TaskInfo.Session(startTime, stopTime, Collections.emptyList());
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

    private JPanel createButtonPanel() {
        JPanel panel = new JPanel(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.insets = new Insets(5, 5, 5, 5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.weightx = 1;
        gbc.weighty = 0;

        panel.add(ok, gbc);
        gbc.gridx++;

        panel.add(cancel, gbc);
        gbc.gridx++;

        panel.add(apply, gbc);
        gbc.gridx++;

        return panel;
    }

    private void updateApply() {
        boolean startValid = startDatePicker.isDateSelected() && startTimePicker.isTimeSelected();
        boolean stopValid = !stopPresent.isSelected() || (stopDatePicker.isDateSelected() && stopTimePicker.isTimeSelected());

        apply.setEnabled(startValid && stopValid);
    }
}
