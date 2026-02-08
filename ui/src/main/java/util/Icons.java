package util;

import com.formdev.flatlaf.FlatIconColors;
import com.formdev.flatlaf.extras.FlatSVGIcon;

import javax.swing.*;
import java.util.Objects;

public class Icons {
    // some that I like: https://www.svgrepo.com/collection/zest-interface-icons/

    public static final FlatSVGIcon activeIcon = new FlatSVGIcon(Objects.requireNonNull(Icons.class.getResource("/activity-svgrepo-com.svg"))).derive(24, 24);
    public static final FlatSVGIcon finishIcon = new FlatSVGIcon(Objects.requireNonNull(Icons.class.getResource("/checkmark-svgrepo-com.svg"))).derive(24, 24);
    public static final FlatSVGIcon pendingIcon = new FlatSVGIcon(Objects.requireNonNull(Icons.class.getResource("/system-pending-line-svgrepo-com" + ".svg"))).derive(24, 24);

    public static final FlatSVGIcon openIcon = new FlatSVGIcon(Objects.requireNonNull(Icons.class.getResource("/export-2-svgrepo-com.svg"))).derive(32, 32);
    public static final FlatSVGIcon unspecifiedTaskIcon = new FlatSVGIcon(Objects.requireNonNull(Icons.class.getResource("/brain-illustration-1-svgrepo-com.svg"))).derive(32, 32);
    public static final FlatSVGIcon dailyReportIcon = new FlatSVGIcon(Objects.requireNonNull(Icons.class.getResource("/report-svgrepo-com.svg"))).derive(32, 32);

    public static final FlatSVGIcon searchIcon = new FlatSVGIcon(Icons.class.getResource("/search-svgrepo-com.svg")).derive(24, 24);

    public static final FlatSVGIcon addIcon16 = new FlatSVGIcon(Objects.requireNonNull(Icons.class.getResource("/add-svgrepo-com.svg"))).derive(16, 16);
    public static final FlatSVGIcon addIcon24 = new FlatSVGIcon(Objects.requireNonNull(Icons.class.getResource("/add-svgrepo-com.svg"))).derive(24, 24);
    public static final FlatSVGIcon addIcon32 = new FlatSVGIcon(Objects.requireNonNull(Icons.class.getResource("/add-svgrepo-com.svg"))).derive(32, 32);

    public static final FlatSVGIcon removeIcon16 = new FlatSVGIcon(Objects.requireNonNull(Icons.class.getResource("/remove-svgrepo-com.svg"))).derive(16, 16);
    public static final FlatSVGIcon removeIcon24 = new FlatSVGIcon(Objects.requireNonNull(Icons.class.getResource("/remove-svgrepo-com.svg"))).derive(24, 24);
    public static final FlatSVGIcon removeIcon32 = new FlatSVGIcon(Objects.requireNonNull(Icons.class.getResource("/remove-svgrepo-com.svg"))).derive(32, 32);

    public static final FlatSVGIcon editIcon16 = new FlatSVGIcon(Objects.requireNonNull(Icons.class.getResource("/edit-svgrepo-com.svg"))).derive(16, 16);

    public static final FlatSVGIcon upIcon16 = new FlatSVGIcon(Objects.requireNonNull(Icons.class.getResource("/up-chevron-svgrepo-com.svg"))).derive(16, 16);
    public static final FlatSVGIcon downIcon16 = new FlatSVGIcon(Objects.requireNonNull(Icons.class.getResource("/down-chevron-svgrepo-com.svg"))).derive(16, 16);

    public static final FlatSVGIcon archiveToggle16 = new FlatSVGIcon(Objects.requireNonNull(Icons.class.getResource("/archive-check-svgrepo-com.svg"))).derive(16, 16);
    public static final FlatSVGIcon archiveUp16 = new FlatSVGIcon(Objects.requireNonNull(Icons.class.getResource("/archive-up-svgrepo-com.svg"))).derive(16, 16);
    public static final FlatSVGIcon archiveDown16 = new FlatSVGIcon(Objects.requireNonNull(Icons.class.getResource("/archive-down-svgrepo-com.svg"))).derive(16, 16);

    static {
        activeIcon.setColorFilter(new FlatSVGIcon.ColorFilter(color -> UIManager.getColor(FlatIconColors.OBJECTS_GREEN.key)));
        finishIcon.setColorFilter(new FlatSVGIcon.ColorFilter(color -> UIManager.getColor(FlatIconColors.OBJECTS_PURPLE.key)));
        pendingIcon.setColorFilter(new FlatSVGIcon.ColorFilter(color -> UIManager.getColor(FlatIconColors.OBJECTS_YELLOW.key)));

        // pull the color from the theme and also listen for theme changes
        addIcon16.setColorFilter(new FlatSVGIcon.ColorFilter(color -> UIManager.getColor("Button.foreground")));
        addIcon24.setColorFilter(new FlatSVGIcon.ColorFilter(color -> UIManager.getColor("Button.foreground")));
        addIcon32.setColorFilter(new FlatSVGIcon.ColorFilter(color -> UIManager.getColor("Button.foreground")));

        removeIcon16.setColorFilter(new FlatSVGIcon.ColorFilter(color -> UIManager.getColor("Button.foreground")));
        removeIcon24.setColorFilter(new FlatSVGIcon.ColorFilter(color -> UIManager.getColor("Button.foreground")));
        removeIcon32.setColorFilter(new FlatSVGIcon.ColorFilter(color -> UIManager.getColor("Button.foreground")));

        editIcon16.setColorFilter(new FlatSVGIcon.ColorFilter(color -> UIManager.getColor("Button.foreground")));

        archiveToggle16.setColorFilter(new FlatSVGIcon.ColorFilter(color -> UIManager.getColor("Button.foreground")));
        archiveUp16.setColorFilter(new FlatSVGIcon.ColorFilter(color -> UIManager.getColor("Button.foreground")));
        archiveDown16.setColorFilter(new FlatSVGIcon.ColorFilter(color -> UIManager.getColor("Button.foreground")));
    }
}
