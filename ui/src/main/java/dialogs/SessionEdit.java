package dialogs;

import com.formdev.flatlaf.FlatClientProperties;
import com.formdev.flatlaf.extras.FlatSVGIcon;
import taskglacier.MainFrame;
import util.LabeledComponent;

import javax.swing.*;
import java.awt.*;

public class SessionEdit extends JDialog {
    public enum Type {
        ADD, EDIT
    }
    public SessionEdit(MainFrame mainFrame, Type type) {
        setModal(true);

        setTitle(type == Type.ADD ? "Add Session" : "Edit Session");

        JFormattedTextField startDate = new JFormattedTextField();
        JFormattedTextField stopDate = new JFormattedTextField();

        JFormattedTextField startTime = new JFormattedTextField();
        JFormattedTextField stopTime = new JFormattedTextField();

        FlatSVGIcon calendarIcon = new FlatSVGIcon(getClass().getResource("/raven/datetime/icon/calendar.svg")).derive(24, 24);

        JButton startDateSelect = new JButton(calendarIcon);
        JButton stopDateSelect = new JButton(calendarIcon);

        startDate.putClientProperty(FlatClientProperties.TEXT_FIELD_TRAILING_COMPONENT, startDateSelect);
        stopDate.putClientProperty(FlatClientProperties.TEXT_FIELD_TRAILING_COMPONENT, stopDateSelect);

        startDateSelect.addActionListener(e -> {

        });

        stopDateSelect.addActionListener(e -> {

        });

        FlatSVGIcon clockIcon = new FlatSVGIcon(getClass().getResource("/raven/datetime/icon/clock.svg")).derive(24, 24);

        JButton startTimeSelect = new JButton(clockIcon);
        JButton stopTimeSelect = new JButton(clockIcon);

        startTime.putClientProperty(FlatClientProperties.TEXT_FIELD_TRAILING_COMPONENT, startTimeSelect);
        stopTime.putClientProperty(FlatClientProperties.TEXT_FIELD_TRAILING_COMPONENT, stopTimeSelect);

        startTimeSelect.addActionListener(e -> {

        });

        stopTimeSelect.addActionListener(e -> {

        });


        JCheckBox stopPresent = new JCheckBox("Include Stop");

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
        gbc.fill = GridBagConstraints.BOTH;

        add(start, gbc);
        gbc.gridx++;

        add(stop, gbc);
        gbc.gridy++;

        JButton save = new JButton("Save");

        gbc.weightx = 0;
        gbc.weighty = 0;

        gbc.anchor = GridBagConstraints.SOUTHEAST;
        gbc.fill = GridBagConstraints.NONE;

        add(save, gbc);

        pack();

        // center on the main frame
        setLocationRelativeTo(mainFrame);

        stopPresent.addActionListener(e -> {
            stopDate.setEnabled(stopPresent.isSelected());
            stopDateSelect.setEnabled(stopPresent.isSelected());

            stopTime.setEnabled(stopPresent.isSelected());
            stopTimeSelect.setEnabled(stopPresent.isSelected());
        });

        stopDate.setEnabled(false);
        stopDateSelect.setEnabled(false);

        stopTime.setEnabled(false);
        stopTimeSelect.setEnabled(false);
    }
}
